# Xtion Pro Live Windows 11対応ドライバー開発ドキュメント

## 1. 概要

### 1.1 目的
ASUS Xtion Pro LiveをWindows 11（Ryzen AI 9 HX 370）で動作させるためのカスタムドライバー開発

### 1.2 現状の課題
- 公式サポート：2014年終了
- 最終対応OS：Windows 8.1
- Windows 10/11用公式ドライバー：存在しない
- 既存OpenNI2実装：Windows 11で動作不安定

## 2. ハードウェア仕様

### 2.1 Xtion Pro Live
- **チップセット**：PrimeSense PS1080
- **接続**：USB 2.0
- **VID/PID**：0x1d27:0x0601（通常）、0x1d27:0x0600（ファームウェア更新モード）
- **RGBカメラ**：640×480 @ 30fps
- **深度センサー**：赤外線投影方式、640×480 @ 30fps
- **動作範囲**：0.8m～3.5m
- **視野角**：58° H、45° V、70° D
- **必須ファームウェア**：v5.8.22

## 3. アーキテクチャ

### 3.1 システム構成

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
│  OpenNI2 API    │ (オプション)
└────────┬────────┘
         │
┌────────▼────────┐
│  Application    │
└─────────────────┘
```

### 3.2 主要コンポーネント

1. **USBCommunication**: USB通信レイヤー（libusb使用）
2. **ProtocolHandler**: PrimeSense PS1080プロトコル実装
3. **DepthStream**: 深度データストリーム管理
4. **ColorStream**: RGBカラーストリーム管理
5. **IRStream**: 赤外線ストリーム管理
6. **XtionDevice**: 高レベルデバイスAPI

## 4. USBプロトコル

### 4.1 エンドポイント構成

FW 3.0/4.0 以降（OpenNI2 `XnDeviceSensorIO.cpp` より）:

```c
// コマンド/応答: エンドポイント0の vendor control transfer
//   送信: bmRequestType=0x40, bRequest=0, wValue=0, wIndex=0
//   受信: bmRequestType=0xC0, bRequest=0, wValue=0, wIndex=0（応答が来るまでポーリング）
#define EP_DEPTH_IN     0x81  // 深度
#define EP_IMAGE_IN     0x82  // カラー/IR
#define EP_MISC_IN      0x83  // その他
// Interface 0: Alt 0 = ISO, Alt 1 = BULK
```

### 4.2 コマンドプロトコル

詳細は [WINDOWS11_LIBUSB_PORT.md](WINDOWS11_LIBUSB_PORT.md) を参照。

```c
// リクエストヘッダ（リトルエンディアン, 8バイト）
struct { uint16_t magic /* 0x4d47 */; uint16_t size /* ペイロードのワード数 */;
         uint16_t opcode; uint16_t id; };
// 応答: 同形式ヘッダ（magic 0x4252）+ uint16_t errorCode（0=ACK, 13=NOT_READY）+ データ

// 主な opcode
// GET_VERSION=0, KEEP_ALIVE=1, GET_PARAM=2, SET_PARAM=3, GET_FIXED_PARAMS=4, GET_SERIAL_NUMBER=37
// ストリーム開始/停止は専用 opcode ではなく SET_PARAM でストリームモードを設定する
```

### 4.3 データフォーマット

#### 深度データ
- パケット種別：0x7100（開始）/ 0x7200（途中）/ 0x7500（終了）
- 形式：圧縮/パック済みの shift 値（ホスト側で固定パラメータからの変換表により mm に変換）
- 無効値：0x0000

#### RGBデータ
- 形式：YUV422またはRGB888
- 解像度：640×480
- FPS：30

## 5. 開発ロードマップ

### Phase 1: USB通信解析（1週間）
- [x] プロジェクト構造作成
- [ ] Wireshark + USBPcap環境構築
- [ ] 既存Windows 8ドライバーでの通信キャプチャ
- [ ] 初期化シーケンス記録
- [ ] データ転送プロトコル解析

### Phase 2: 基本ドライバー実装（2週間）
- [x] libusb環境構築
- [x] デバイス検出・列挙機能
- [x] USB接続・切断処理
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

## 6. 既知の問題と回避策

| 問題 | 原因 | 回避策 |
|------|------|--------|
| USB 3.0での認識不良 | 電力供給不足 | USB 2.0ポート使用、電源付きハブ使用 |
| snd_usb_audioの競合 | マイクデバイスとして認識 | ドライバーブラックリスト追加 |
| 深度・カラー同期ズレ | タイムスタンプ差異 | バッファリング＋タイムスタンプ補正 |
| Windows 11でのBSOD | 未署名ドライバー | テストモード有効化、自己署名証明書 |
| AMD USB Controller問題 | IOMMU設定 | BIOS設定変更、互換モード |

## 7. デバッグ手順

### 7.1 USB通信の解析

1. USBPcapのインストール
```powershell
# Wiresharkと一緒にUSBPcapをインストール
winget install Wireshark.Wireshark
```

2. キャプチャの開始
   - Wiresharkを起動
   - USBPcapインターフェースを選択
   - フィルタ: `usb.idVendor == 0x1d27`

3. 分析ポイント
   - コントロール転送のセットアップパケット
   - バルク転送のデータパターン
   - タイミングとレイテンシ

### 7.2 ログ出力

```cpp
// デバッグビルド時にUSB通信をログ出力
#ifdef DEBUG
    #define USB_LOG(fmt, ...) fprintf(stderr, "[USB] " fmt "\n", ##__VA_ARGS__)
#else
    #define USB_LOG(fmt, ...)
#endif
```

## 8. パフォーマンス目標

- **フレームレート**: 30fps（深度＋カラー）
- **CPU使用率**: < 20%
- **メモリ使用量**: < 200MB
- **レイテンシ**: < 100ms

## 9. テスト計画

### 9.1 単体テスト
- USB接続/切断 100回連続
- 各ストリーム個別動作
- メモリリーク検証（24時間連続）
- 例外処理確認

### 9.2 統合テスト
- OpenNI2 SimpleViewer動作
- NiTE2スケルトントラッキング
- 複数アプリケーション同時起動
- システムスリープ/復帰

### 9.3 パフォーマンステスト
- 30fps安定性（1時間）
- CPU使用率測定
- メモリ使用量測定
- レイテンシ測定

## 10. 参考資料

- [xqms/xtion](https://github.com/xqms/xtion) - Linux kernel driver
- [OpenNI2](https://structure.io/openni) - Cross-platform framework
- [libfreenect](https://github.com/OpenKinect/libfreenect) - Kinect v1 driver
- [PrimeSense Protocol Documentation](https://openkinect.org/wiki/Protocol_Documentation)
- [libusb API](https://libusb.sourceforge.io/api-1.0/)

## 11. ライセンス

このプロジェクトはApache License 2.0の下で公開予定です。

依存ライブラリのライセンス：
- OpenNI2: Apache License 2.0
- libusb: LGPL v2.1
