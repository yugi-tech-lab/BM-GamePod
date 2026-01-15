# BM-GamePod
BM(ブレッドメイカー)シリーズの誰でも簡単に作れるシンプルなゲーム機です。


# 自作小型ゲーム機プロジェクト

![完成写真](images/GamePod.jpg)

## 概要
本リポジトリは、自作した小型ゲーム機の  
- **ケース(3Dプリンタ用 / 素材：PLA)**
- **ゲームのソースコード**
- **キャラクタの変更方法の紹介()**

を公開するものです。

自分でケースを出力し、ソフトを書き込むことで、  
オリジナルの小型ゲーム機として動作します。
(キット購入の方は、ケースの出力は不要です)
ハードの説明と組み立て方は、ProtoPedia GamePodの記事(<a href="https://twitter.com/YugiTechLab?ref_src=twsrc%5Etfw">@YugiTechLab</a>)を参考にしてください。


---
## 本リポジトリの構成

```text
├─hard/
│  └─model/
│     └─case/
│        └─ BM_GamePod_case.stl
├─soft/
│  ├─ devtools/
│  │    ├─ config.js
│  │    ├─ DotImageEditor.html
│  │    └─ GamePrompter.html
│  └─ GamePod.ino
└─ README.md
```
---

## ケース（3Dモデル）

![ケース](images/case.png)

- `hard/model/case` ディレクトリに STL ファイルがあります
- 3DプリンターはBumboo LabのP1Sで印刷実績があります。

---
## ゲームのソースコード

- ゲームのソースコードはすべてGamePod.inoに記載されています。
- ソースコードをArduino Nanoに書き込むときは、Arduino ideを使用しています。

### キャラクターの書き換え方法
---

---

# GitHubで公開（GitHub Pages）

このリポジトリには、Arduino用スケッチとは別に **Webツール（HTMLアプリ）** が入っています。

- `soft/devtools/GamePrompter.html`
- `soft/devtools/DotImageEditor.html`
- `soft/devtools/config.js`（PC送信先などの設定）

## 1. GitHubへアップロード
> ※ZIPに `.git/` が入っている場合は、GitHubに上げる前に **削除** してください（PC内の履歴なので不要です）

1) GitHubで新規リポジトリを作成（例: `BM-GamePod`）  
2) このフォルダの中身をそのままコミットして push

## 2. GitHub Pages を有効化
1) リポジトリの **Settings → Pages**  
2) **Deploy from a branch** を選択  
3) Branch: `main` / Folder: `/(root)` を選んで Save

公開URLは次の形になります：

- `https://<ユーザー名>.github.io/<リポジトリ名>/`

このリポジトリには公開用の入口として `index.html` を追加してあります。  
公開後はこのURLを開くだけで DevTools に入れます。

## 3. DevTools のURL（直接リンク）
- `https://<ユーザー名>.github.io/<リポジトリ名>/soft/devtools/GamePrompter.html`
- `https://<ユーザー名>.github.io/<リポジトリ名>/soft/devtools/DotImageEditor.html`

## 4. ページ内に「共有リンク」を表示（追加済み）
各HTMLの右下に小さなボックスを追加しています：

- **このページ**（今開いているURL）
- **URLコピー**（共有用にコピー）
- `config.js` の `GITHUB_REPO_URL` を設定すると **GitHub** リンクも表示できます

### 設定（任意）
`soft/devtools/config.js` を編集：
- `PC_URL`：PC送信先のURL
- `GITHUB_REPO_URL`：リポジトリURL（空なら非表示）
