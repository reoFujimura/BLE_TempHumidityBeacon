# nRF52840の学習記録

今回は、Adafruit Feather nRF52840 Expressを用いて、以下システム向けのファームウェアを開発しました。

## システム構成

開発対象のシステム構成は以下の通りです。

<img width="500" alt="システム構成図" src="https://github.com/user-attachments/assets/ee6a5294-7980-420e-b2f5-a129eb41e955">


## 動作確認結果

開発したファームウェアの動作を確認するため、iPhoneのnRF Connectアプリを使用してBLEビーコンをモニタリングしました。
結果は以下の通りです。

<img width="500" alt="nRFConnect確認結果" src="https://github.com/user-attachments/assets/866afcd1-8e15-4e1a-bb45-0e8b8656fd9f">


## ファームウェアの構成

簡単ですが、今回作成したの構成は以下の通りです。

![class](https://github.com/user-attachments/assets/adeb010d-61a2-43ab-888f-4f18c40b7237)

----

## 課題と次のステップ

本プロジェクトでは以下の課題が残っています。

これらを解決しつつ、新たなシステムの開発に取り組んでいます。

### 残課題
- クラスに分けられていない処理がある
- 各ログレベル毎に専用のログ関数を作成する

### 次のシステム構成図

BLE通信について学習するため、以下システムの開発を進めています。

<img width="1500" alt="現在開発中のシステム構成図" src="https://github.com/user-attachments/assets/f367268d-d136-4638-81c4-012b516cb3bd">
