# BM-GamePod

BM-GamePod のハード（STL）とソフト（Arduinoスケッチ／Webツール）をまとめたリポジトリです。

---

## Links

### Tools（ブラウザで動く）
- OLED Dot Editor  
  https://yugi-tech-lab.github.io/BM-GamePod/sw/tools/oled_dot_editor.html

（GitHub上のファイルはこちら）  
- https://github.com/yugi-tech-lab/BM-GamePod/blob/main/sw/tools/oled_dot_editor.html

---

## Firmware（Arduino）

- スケッチ：`sw/bm_gamepod/bm_gamepod.ino`

### ビルド/書き込み
1. Arduino IDE で `sw/bm_gamepod/bm_gamepod.ino` を開く
2. ボード/ポートを選択して書き込み

※ 依存ライブラリや配線はスケッチ内コメントを参照してください。

---

## Tools 設定

- 設定ファイル：`sw/tools/config.js`

例：PC送信先などをここで調整します（必要な場合のみ編集）。
```js
// sw/tools/config.js
// PC送信用設定（ここだけ編集すればOK）
const PC_URL = "http://192.168.11.7:8000";
```

---

## Hardware

- STL：`hw/stl/bm_gamepod.stl`

---

## Folder Structure


```
BM-GamePod/
├─ index.html
├─ LICENSE
├─ README.md
├─ .gitignore
├─ .gitattributes
├─ assets/
│  ├─ img/
│  └─ icons/
├─ docs/
│  ├─ wiring.md
│  ├─ bom.md
│  └─ howto.md
├─ hw/
│  ├─ README.md
│  └─ stl/
│     └─ bm_gamepod.stl
└─ sw/
   ├─ tools/
   │  ├─ README.md
   │  ├─ config.js
   │  └─ oled_dot_editor.html
   └─ bm_gamepod/
      ├─ README.md
      └─ bm_gamepod.ino
```

---

## License

TBD（必要に応じて追記してください）
- 例：MIT / Apache-2.0 / CC BY など

---

## Author / Community

- yugi-tech-lab  
  https://github.com/yugi-tech-lab
