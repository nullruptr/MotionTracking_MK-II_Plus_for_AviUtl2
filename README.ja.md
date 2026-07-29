# MotionTracking_MK-II_Plus_for_AviUtl2

AviUtl ExEdit2 でオブジェクトトラッキングを行うプラグイン

## 必要動作環境

- AVX2をサポートしたCPU
- Windows 10以降のOS
- DirectX11.3 が利用できる環境
- AviUtl ExEdit2 version 2.0beta48 以降
- AviUtl ExEdit2 version 2.1.0 にて動作確認済み。

## インストール

zip内の.aux2ファイルを`AviUtl ExEdit2 が汎用プラグインを読み込むお好きなディレクトリ`に置いてください。
MotionTracking_modelディレクトリは、`MotionTrackingMKIIPlusforAviUtl2.aux2`が存在するディレクトリと同じ場所に新規作成してください。

AviUtl ExEdit2 の`表示`メニューに"MotionTracking MK-II Plus for AviUtl2"が追加されていたら成功です。

また、機械学習を用いたトラッキングアルゴリズムであるDaSiamRPN, Nano, Vitを使用する場合、追加で作業が必要です。(学習データを同梱することが困難であるため)

### DaSiamRPN 用

[こちらのURL](https://github.com/opencv/opencv/blob/4.x/samples/dnn/dasiamrpn_tracker.cpp)のソースコードにコメントアウトとして記載されているURLより

- dasiamrpn_model.onnx
- dasiamrpn_kernel_r1.onnx
- dasiamrpn_kernel_cls1.onnx

をダウンロードし、`MotionTracking_modelディレクトリ内`に置いてください。

### Nano 用

[こちらのURL](https://github.com/HonglinChu/SiamTrackers/tree/18b7791360acb3f6d276d47376a6f1ed516f1628/NanoTrack/models/nanotrackv2)より

- nanotrack_backbone_sim.onnx
- nanotrack_head_sim.onnx

をダウンロードし、`MotionTracking_modelディレクトリ内`に置いてください。

### Vit 用

[こちらのURL](https://github.com/opencv/opencv_extra/blob/4.x/testdata/dnn/onnx/models/vitTracker.onnx)より

- vitTracker.onnx

をダウンロードし、`MotionTracking_modelディレクトリ内`に置いてください。

## ヘルパープラグイン

> [!Important]
> ヘルパープラグインについては、アップデートで対応予定です。現段階では、未実装です。

一つのAUX2ファイルに2つのヘルパープラグインを同梱しています。

1. Pre-track: HSV Cvt
   RGB画像をHSVに変換し、それをRGB画像の様に表示させます。また、HSVチャンネルの一つのみを表示させることができます。

2. Pre-track: BGSubtraction  
   背景から動く物体を分離することを目的とするプラグインです。分離したRGB画像を出力するか、グレースケールのマスクを出力することができます。Rangeの値を大きくしすぎた場合、メモリ不足を引き起こす可能性がありますので、ご注意ください。

## ヘルプ

### MotionTracking MK-II Plus

#### 使用方法

0. トラッキングしたいフレームの範囲を選択する。
1. 「Select Object」ボタンをクリックし、ポップアップウィンドウ内で追跡するオブジェクトをドラッグして指定する。ポップアップウィンドウをxまたはF3で閉じる。
2. 「Analyze」ボタンをクリックし、解析終了まで待つ。解析を中断するには、x ボタンをクリックする。
3. 「View Result」ボタンをクリックし、結果を確認。もし結果が良かった場合は、「Invert Position」オプションを用途によって有効化し、「Insert Object」をクリックしてInset Object または、Object ファイルを保存する。よくなかった場合は、「Clear Result」をクリックして、結果を削除し、ステップ0か1に戻る。

#### Export Object File

正常な結果に1フレームのみ挟まれたエラーの自動補正機能が搭載されています。
CJKファイル名もサポートされています。

#### オプション

##### プルダウンメニューのオプション

###### Method

解析で使用するアルゴリズムを指定します

1. Multi Instance Learning
2. KCF
3. CSRT
4. DaSiamRPN
5. Nano
6. Vit

###### Hue

Object SelectionやView Resultで表示される矩形の色相を指定します

##### Insert Object のオプション

- As Sub-filter/部分フィルタ？ : 部分フィルタとして出力するか
- Invert Position : トラッキング結果の座標を反転させるか
- Ignore Aspect Ratio : アスペクト比を無視し、拡大率で出力するか

### Pre-track:BGSubtraction

> [!Important]
> Pre-track:BGSubtraction については、アップデートで対応予定です。現段階では、未実装です。

#### 共通パラメータ

- Range : 現在のフレームの前後何フレームを解析に使用するか [30]
- Shadow : 1= 影の検出を有効化 [0]

#### MOG2のみ

- NMix : 背景モデルのガウス成分の数 [5]
- BG% : 背景比率 [70%]

#### KNNのみ

- d2T : あるピクセルがそのサンプルに近いかどうかを判断するための、ピクセルとサンプルの距離の2乗のしきい値

## ソースからのビルド

Linux(Docker)上でMinGWを用いてビルドする場合は、`.github/workflows/build.yml`または、[Dockerfile](https://github.com/nullruptr/MotionTracking_MK-II_Plus_for_AviUtl2/tree/master/docker)をご覧ください。

以下では、Windows上でMSVCを用いてビルドする手順(Release版/Debug版共通)を説明します。

### 必要なもの

- Windows 10/11
- [Visual Studio 2022](https://visualstudio.microsoft.com/ja/) (「C++によるデスクトップ開発」ワークロード。MSVC v143 ツールセット、Windows 10/11 SDK を含む)
- [Git](https://git-scm.com/)(サブモジュール取得のため)
- [Python](https://www.python.org/) 3.13 以上
- [Poetry](https://python-poetry.org/)(Pythonの依存関係・仮想環境管理。この後の手順でConanをインストールするために使用)
- [CMake](https://cmake.org/) 3.20 以上

### 1. リポジトリの取得

サブモジュール(`src/aviutl2_sdk`)を含めて取得してください。

```bash
git clone --recursive https://github.com/nullruptr/MotionTracking_MK-II_Plus_for_AviUtl2.git
cd MotionTracking_MK-II_Plus_for_AviUtl2
```

すでにサブモジュールなしでクローン済みの場合は、以下で取得できます。

```bash
git submodule update --init --recursive
```

### 2. Poetry のインストール

公式のインストーラーを使用してインストールしてください。([Poetry公式ドキュメント](https://python-poetry.org/docs/#installing-with-the-official-installer))

```powershell
(Invoke-WebRequest -Uri https://install.python-poetry.org -UseBasicParsing).Content | py -
```

インストール後、`poetry --version` が実行できることを確認してください。PATHが通っていない場合は、シェルを再起動するか、PATHを追加してください。

### 3. Conan のセットアップ

リポジトリのルートで、以下を実行し、Poetryの仮想環境(Conanを含む)を作成します。

```bash
poetry install
```

これにより、`pyproject.toml`に記載されている`conan`(Conan 2系)が仮想環境内にインストールされます。以降のConanコマンドは`poetry run conan ...`のように実行します。

Conanを初めて使用する場合(`~/.conan2/profiles/default`が存在しない場合)は、ビルド環境用(build context用)のデフォルトプロファイルを1度だけ作成してください。

```bash
poetry run conan profile detect --force
```

なお、実際にビルドで使用するホスト側(host context)のプロファイルは、リポジトリに同梱されている以下のファイルを使用します(手動での作成は不要です)。

- Release版: [`profiles/msvc-release`](profiles/msvc-release)
- Debug版: [`profiles/msvc-debug`](profiles/msvc-debug)

### 4. MSVC でのビルド

依存ライブラリ(OpenCV)のビルドを含むため、初回は時間がかかります(30分以上かかる場合があります)。
ビルドは「x64 Native Tools Command Prompt for VS 2022」または「Developer PowerShell for VS 2022」から実行することを推奨します。

#### make のセットアップ

make は、[Make for Windows](https://gnuwin32.sourceforge.net/packages/make.htm)を利用します。
インストールするものは、`Description`にある、`Complete package, except sources`の、`Setup` です。
ダウンロードが完了したら、インストーラから make をインストールします。
インストール後、パスを通してセットアップしてください。
make のセットアップ完了後、下記内容を実行します。

```bash
cd src

# Release 版
make compile

# Debug 版
make compile-debug
```

`make compile` は、Release版のビルドに加えて、clangd 用の `compile_commands.json` の生成(`compdb`)も行います。

### 5. ビルド成果物

ビルドが成功すると、以下の場所に`.aux2`ファイルが生成されます。

- Release版: `src/build/build/Release/MotionTrackingMKIIPlusforAviUtl2.aux2`
- Debug版: `src/build-debug/build/Debug/MotionTrackingMKIIPlusforAviUtl2.aux2`

生成された`.aux2`ファイルと、`MotionTracking_model`ディレクトリを、AviUtl ExEdit2 が汎用プラグインを読み込むディレクトリに配置してください(詳細は上記「インストール」の項を参照)。

### 6. デバッグの方法

デバッグは以下の手順で行います。

1. ビルドで生成された Debug版の `MotionTrackingMKIIPlusforAviUtl2.aux2`と`MotionTrackingMKIIPlusforAviUtl2.pdb`をプラグインを読み込むディレクトリにコピー
2. `src/build-debug/build/MotionTrackingMKIIPlusforAviUtl2.sln` をVisual Studioで開く
3. ソリューションエクスプローラーで、`MotionTrackingMKIIPlusforAviUtl2`を右クリック
4. 右クリックメニューより、`スタートアップ プロジェクトに設定` をクリック
5. `MotionTrackingMKIIPlusforAviUtl2`が太字になっていることを確認する
6. 再度、`MotionTrackingMKIIPlusforAviUtl2`を右クリック -> プロパティ -> `MotionTrackingMKIIPlusforAviUtl2 プロパティ ページ` を開く
7. 画面左のツリーを、構成プロパティ -> デバッグの順で開く
8. コマンドに `aviutl2.exe` が存在するフルパスを入力(AviUtl2 のデフォルト設定なら `C:\Program Files\AviUtl2\aviutl2.exe`)
9. 作業ディレクトリに `aviutl2.exe` が存在するフォルダパスを入力(AviUtl2 のデフォルト設定なら、 `C:\Program Files\AviUtl2`)
10. `プロパティ ページ` 変更内容を適用し、閉じる
11. F5 でデバッグを開始

## バグ報告

- [GitHub](https://github.com/nullruptr/MotionTracking_MK-II_Plus_for_AviUtl2)
