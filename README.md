# Xtion Pro Live Windows 11 Driver

ASUS Xtion Pro LiveをWindows 11（特にRyzen AI 9 HX 370）で動作させるためのカスタムドライバーです。

## 概要

ASUS Xtion Pro Liveは2014年に公式サポートが終了し、Windows 8.1が最終対応OSとなっています。このプロジェクトは、Windows 10/11で安定動作する非公式ドライバーを提供することを目的としています。

## ハードウェア仕様

- **チップセット**: PrimeSense PS1080
- **接続**: USB 2.0
- **VID/PID**: 0x1d27:0x0601（通常）/ 0x1d27:0x0600（ファームウェア更新モード）
- **RGBカメラ**: 640×480 @ 30fps
- **深度センサー**: 赤外線投影方式、640×480 @ 30fps
- **動作範囲**: 0.8m～3.5m
- **必須ファームウェア**: v5.8.22

## アーキテクチャ

```
┌─────────────────┐
│ Xtion Pro Live  │
└────────┬────────┘
         │ USB 2.0
┌────────▼────────┐
│     libusb      │
└────────┬────────┘
         │
┌────────▼────────┐
│ Custom Driver   │
│  (C++/WinUSB)   │
└────────┬────────┘
         │
┌────────▼────────┐
│  OpenNI2 API    │
└────────┬────────┘
         │
┌────────▼────────┐
│  Application    │
└─────────────────┘
```

## 必要な環境

### 開発ツール
- Visual Studio 2022 Community以上（Windows）
- CMake 3.20以上
- Git for Windows
- Python 3.8以上（ビルドスクリプト用）

### ライブラリ
- libusb-1.0 (v1.0.20以降)
- OpenNI2 (v2.2.0.33) - オプション
- Visual C++ Runtime (2015以降)
- libjpeg-turbo - オプション

## セットアップ

### Windows

```powershell
# vcpkgのインストール
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install

# 必要なライブラリのインストール
.\vcpkg install libusb:x64-windows
.\vcpkg install libjpeg-turbo:x64-windows
.\vcpkg install glfw3:x64-windows
```

### OpenNI2のセットアップ（オプション）

```powershell
# OpenNI2のダウンロードとインストール
# https://structure.io/openni からダウンロード

# 環境変数の設定
[System.Environment]::SetEnvironmentVariable("OPENNI2_INCLUDE", "C:\OpenNI2\Include", "User")
[System.Environment]::SetEnvironmentVariable("OPENNI2_REDIST", "C:\OpenNI2\Redist", "User")
```

## ビルド

```bash
# プロジェクトのクローン
git clone <repository-url>
cd XtionProLive_Windows11_compatible_driver_development

# ビルドディレクトリの作成
mkdir build
cd build

# CMakeの実行
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake

# ビルド
cmake --build . --config Release
```

## インストール

```bash
# OpenNI2統合の場合
cmake --install . --config Release

# ドライバーは自動的にOpenNI2/Driversディレクトリにインストールされます
```

## 使用方法

### 基本的な使用例

```cpp
#include "XtionDevice.h"

// デバイスの初期化
XtionDevice device;
if (device.open()) {
    // 深度ストリームの開始
    device.startDepthStream();

    // フレームの取得
    DepthFrame frame;
    if (device.readDepthFrame(frame)) {
        // データ処理
    }

    device.close();
}
```

## 開発ロードマップ

### Phase 1: USB通信解析（1週間）
- [x] プロジェクト構造の作成
- [ ] Wireshark + USBPcap環境構築
- [ ] 既存Windows 8ドライバーでの通信キャプチャ
- [ ] 初期化シーケンス記録
- [ ] データ転送プロトコル解析

### Phase 2: 基本ドライバー実装（2週間）
- [ ] libusb環境構築
- [ ] デバイス検出・列挙機能
- [ ] USB接続・切断処理
- [ ] 基本的なコマンド送信
- [ ] RAWデータ取得

### Phase 3: OpenNI2統合（2週間）
- [ ] OpenNI2ドライバーインターフェース実装
- [ ] DepthStream実装
- [ ] ColorStream実装
- [ ] IRStream実装
- [ ] 深度・カラー同期機構

### Phase 4: 最適化・安定化（1週間）
- [ ] マルチスレッド最適化
- [ ] メモリ使用量最適化
- [ ] エラーハンドリング強化
- [ ] ログ機能実装
- [ ] パフォーマンステスト

## 既知の問題

| 問題 | 原因 | 回避策 |
|------|------|--------|
| USB 3.0での認識不良 | 電力供給不足 | USB 2.0ポート使用、電源付きハブ使用 |
| Windows 11でのBSOD | 未署名ドライバー | テストモード有効化、自己署名証明書 |
| AMD USB Controller問題 | IOMMU設定 | BIOS設定変更、互換モード |

## トラブルシューティング

### デバイスが認識されない
- デバイスマネージャーで確認
- libusb-win32ドライバーのインストール確認
- USB 2.0ポートの使用

### 深度画像が取得できない
- ファームウェアv5.8.22への更新
- USB 2.0ポートの使用確認

### OpenNI2で検出されない
- ドライバーファイルがOpenNI2/Driversにあることを確認
- 依存DLLの確認

## 貢献

プルリクエストを歓迎します。大きな変更の場合は、まずissueを開いて変更内容を議論してください。

## ライセンス

このプロジェクトは検討中です（Apache License 2.0を推奨）。

### 依存ライブラリのライセンス
- OpenNI2: Apache License 2.0
- libusb: LGPL v2.1

## 参考資料

- [OpenNI2 API Documentation](http://openkinect.github.io/libfreenect2/)
- [PrimeSense Protocol Wiki](https://openkinect.org/wiki/Protocol_Documentation)
- [libusb Programming Guide](https://libusb.sourceforge.io/api-1.0/)
- [Windows Driver Development](https://docs.microsoft.com/en-us/windows-hardware/drivers/)

## 謝辞

このプロジェクトは以下のオープンソースプロジェクトを参考にしています：
- xqms/xtion (Linux kernel driver)
- OpenNI2 (Occipital)
- libfreenect (OpenKinect)
