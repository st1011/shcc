# shcc Compiler
C言語（ライクな）コンパイラ

[低レイヤを知りたい人のための Cコンパイラ作成入門](https://www.sigbus.info/compilerbook/) をベースに作成

## ビルド

下記のコマンドでビルドを実行します。

    $ make

下記のコマンドでテストを実行します。

    $ make test

## コンパイラの機能

### 対応済みの構文

- 四則演算（+-*/）
- Modulo（%）
- 括弧（'(' ')'）
- 比較演算子（== != < <= > >=）
- 変数作成
- 代入（=）
- ステートメント終端（;）
- return
- 関数定義、呼び出し
- ブロック（{ }）
- 単項演算子('+' '-')
- 制御構文(if-else for)

### オプション

```
 -test      コンパイラの内部機能のテスト行ないます。
            このオプションを指定された場合コンパイルは実行されません。
 -dumptoken ソースコードをトークナイズした結果を併せて出力します。
```

## 参考文献との差異
進捗状況内での、参考文献との差異（最終的には変更しているかも）
- 参考文献ではASCIIをそのまま使用しているトークンもenumとして定義
- codeもvectorで保存

