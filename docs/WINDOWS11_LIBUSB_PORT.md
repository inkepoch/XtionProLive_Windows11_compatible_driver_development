# OpenNI2 を libusb + WinUSB で Windows 11 に移植する手順（調査結果）

## 結論

- Windows 11 で Xtion Pro Live が動かない主因は、OpenNI2 の Windows 版 USB 層が
  旧 PrimeSense カーネルドライバ（psdrv3）専用の IOCTL に依存していること。
- OpenNI2 の Linux 版 USB 層は **libusb のみ** で実装されており、udev は任意（`XN_USE_UDEV`）。
- Win32 版と Linux 版の USB 層は公開 API（`xnUSB*`）が完全に一致している。
- よって **XnLib の USB 実装を Linux 版（libusb）に差し替え、デバイスに WinUSB を割り当てる** ことで、
  自作カーネルドライバ・ドライバ署名なしで PS1080 ドライバ全体（プロトコル、深度変換、画像処理）を流用できる見込み。

## 根拠（OpenNI2 ソース: structureio/OpenNI2 master）

| 項目 | 内容 |
|------|------|
| Win32 USB 層 | `ThirdParty/PSCommon/XnLib/Source/Win32/XnUSBWin32.cpp` が `GUID_CLASS_PSDRV_USB` と `IOCTL_PSDRV_*` を使用 → psdrv3 が無いと動かない |
| Linux USB 層 | `ThirdParty/PSCommon/XnLib/Source/Linux/XnLinuxUSB.cpp` は `libusb_*` のみ。udev 部分は `#ifdef XN_USE_UDEV` で無効化可能 |
| API 互換 | 両ファイルの `XN_C_API` 関数は同一（Linux 側に内部関数 `xnUSBOpenDeviceImpl` が 1 つ多いだけ） |
| PS1080 側の OS 依存 | `XnDeviceEnumeration.cpp`（WAVI 判定・無害）、`XnSensor.cpp`（Win32 は既定で ISO 転送）、`XnDeviceSensorInit.h`（バッファサイズ）程度 |

### プロトコル要点（疎通確認ツールで実装済み）

- コマンド: vendor control transfer（OUT: `bmRequestType=0x40`, `bRequest=0`, `wValue=0`, `wIndex=0`）
- 応答: vendor control transfer（IN: `bmRequestType=0xC0`）をポーリング
- ヘッダ（FW 1.2 以降, LE）: `magic=0x4d47 / size(16bitワード数) / opcode / id`
- 応答ヘッダ: `magic=0x4252 / size / opcode / id` + `errorCode(0=ACK)`
- 主な opcode: `GET_VERSION=0`, `KEEP_ALIVE=1`, `GET_PARAM=2`, `SET_PARAM=3`, `GET_FIXED_PARAMS=4`, `GET_SERIAL_NUMBER=37`（FW 5.4+）
- ストリーム EP（FW 3.0/4.0 以降）: 深度 `0x81`, 画像 `0x82`, その他 `0x83`。Interface 0 の Alt 0=ISO, Alt 1=BULK
- データパケット種別: 深度 `0x7100/0x7200/0x7500`、画像 `0x8100/0x8200/0x8500`

## 手順

### 0. 疎通確認（このリポジトリの `xtion_probe`）

1. [Zadig](https://zadig.akeo.ie/) で `VID 1D27 / PID 0601` に **WinUSB** を割り当てる
   - `Options → List All Devices` を有効化
   - 複合デバイスとして見える場合は **Interface 0**（`MI_00`）を選ぶ
2. このリポジトリをビルドし `xtion_probe.exe` を実行
3. ファームウェアバージョンとチップ ID が表示されれば、Windows 11 + 当該 USB コントローラで通信可能と判断できる

### 1. 依存ライブラリ

```powershell
.\vcpkg install libusb:x64-windows
```

### 2. OpenNI2 の取得

```powershell
git clone https://github.com/structureio/OpenNI2.git
```

### 3. XnLib の USB 実装を差し替え

`ThirdParty/PSCommon/XnLib/Source/XnLib.vcxproj` を編集:

- 削除: `<ClCompile Include="Win32\XnUSBWin32.cpp" />`
- 追加: `<ClCompile Include="Linux\XnLinuxUSB.cpp" />`
- インクルードパスに vcpkg の libusb（`include/libusb-1.0`）を追加
- リンクに `libusb-1.0.lib` を追加（XnLib を使う PS1080 ドライバ側にも）
- `XN_USE_UDEV` は **定義しない**

### 4. PS1080 ドライバ側の調整

- `Source/Drivers/PS1080/Sensor/XnSensor.cpp` の `XN_SENSOR_DEFAULT_USB_INTERFACE` を
  Win32 でも `XN_SENSOR_USB_INTERFACE_BULK_ENDPOINTS` にする
  （WinUSB 経由の ISO 転送は libusb の比較的新しい機能で実績が少ないため、まず BULK を推奨）
- `XnLinuxUSB.cpp` の `#include <libusb-1.0/libusb.h>` は vcpkg の配置に合わせて調整

### 5. 動作確認

- OpenNI2 付属の `NiViewer` で深度・カラー表示を確認

## 既知の制約・未検証事項

- ホットプラグ通知は udev 依存のため無効（起動時の列挙のみ）
- `XnLinuxUSB.cpp` を MSVC でビルドした実績は未確認。`snprintf`/`struct timeval` 等で小修正が必要な可能性あり
- Xtion Pro Live のマイク（USB Audio）インターフェースとの共存は未検証
- Ryzen AI 9 HX 370 搭載機は USB 2.0 専用ポートを持たない可能性が高い。FW 5.8.22 は USB 3.0 対応版なので必須
- 実機テストはまだ行っていない
