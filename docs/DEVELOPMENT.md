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

```c
#define EP_COLOR_IN     0x81  // RGBカラーストリーム (Bulk)
#define EP_DEPTH_IN     0x82  // 深度データストリーム (Isochronous)
#define EP_IR_IN        0x83  // 赤外線ストリーム (Bulk)
#define EP_COMMAND_OUT  0x02  // コマンド送信 (Control)
#define EP_RESPONSE_IN  0x86  // レスポンス受信 (Interrupt)
```

### 4.2 初期化シーケンス

```c
// 1. デバイスリセット
unsigned char reset_cmd[] = {0x05, 0x00, 0x00, 0x01, 0x00, 0x00};

// 2. ファームウェアバージョン確認
unsigned char version_cmd[] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00};

// 3. ストリームモード設定
unsigned char stream_mode[] = {0x05, 0x00, 0x02, 0x03, 0x01, 0x00};

// 4. 深度カメラ初期化
unsigned char depth_init[] = {0x13, 0x00, 0x00, 0x00, 0x00, 0x00};

// 5. RGBカメラ初期化
unsigned char rgb_init[] = {0x14, 0x00, 0x00, 0x00, 0x00, 0x00};
```

### 4.3 データフォーマット

#### 深度データ
- 形式：16bit unsigned integer
- 単位：ミリメートル
- 無効値：0x0000
- 最大値：0xFFFF

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
