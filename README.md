# shcc C ompiler
C言語（ライクな）コンパイラ

[低レイヤを知りたい人のための Cコンパイラ作成入門](https://www.sigbus.info/compilerbook/) をベースに作成

## Build
-----
    $ make

## できること
- 四則演算（+-*/）
- 変数作成
- 代入（=）
- ステートメント終端（;）
- return

## 参考文献との差異
進捗状況内での、参考文献との差異（最終的には変更しているかも）
- 参考文献ではASCIIをそのまま使用しているトークンもenumとして定義
- codeもvectorで保存

