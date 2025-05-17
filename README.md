# PDFSampleFilter2

## 概要

PDF ファイル向けに [IFilter (filter.h) - Win32 apps | Microsoft Learn](https://learn.microsoft.com/en-us/windows/
win32/api/filter/nn-filter-ifilter) を実装します。

`Adobe PDF IFilter 6.0` や `Adobe PDF iFilter 9 for 64-bit platforms` や `Adobe PDF iFilter 11 for 64-bit platforms` がもはや提供されなくなったため、その代替品として開発しました。

Windows 10 またはそれ以降で動作します。 32 ビット版 (x86) と 64 ビット版 (x64) と、両方の DLL を含んでいます。

PDFium ([bblanchon/pdfium-binaries: 📰 Binary distribution of PDFium](https://github.com/bblanchon/pdfium-binaries)) を使用しています。

## 技術情報

以下のうち、いずれか 1 つのインターフェイスを使用して初期化します。

- [IInitializeWithStream (propsys.h) - Win32 apps | Microsoft Learn](https://learn.microsoft.com/ja-jp/windows/win32/api/propsys/nn-propsys-iinitializewithstream)
- [IPersistFile (objidl.h) - Win32 apps | Microsoft Learn](https://learn.microsoft.com/ja-jp/windows/win32/api/objidl/nn-objidl-ipersistfile)
- [IPersistStream (objidl.h) - Win32 apps | Microsoft Learn](https://learn.microsoft.com/ja-jp/windows/win32/api/objidl/nn-objidl-ipersiststream)


[LoadIFilter 関数 (ntquery.h) - Win32 apps | Microsoft Learn](https://learn.microsoft.com/ja-jp/windows/win32/api/ntquery/nf-ntquery-loadifilter) を利用した初期化ができます ([IPersistFile (objidl.h) - Win32 apps | Microsoft Learn](https://learn.microsoft.com/ja-jp/windows/win32/api/objidl/nn-objidl-ipersistfile) を実装しているため)


提供されるプロパティ例はつぎの通りです。

attribute | propType | locale | contents
---|---|---|---
{F29F85E0-4FF9-1068-AB91-08002B27B3D9},2 | Title | 0 | Microsoft Word - 文書 12
{F29F85E0-4FF9-1068-AB91-08002B27B3D9},4 | Author | 0 | KU
{F29F85E0-4FF9-1068-AB91-08002B27B3D9},3 | Subject | 0 | 
{F29F85E0-4FF9-1068-AB91-08002B27B3D9},5 | Keywords | 0 | 
{B725F130-47EF-101A-A5F1-02608C9EEBAC},19 | Search.Contents | 0 | PDF サンプル#1
{B725F130-47EF-101A-A5F1-02608C9EEBAC},19 | Search.Contents | 0 | PDFサンプル#2
{B725F130-47EF-101A-A5F1-02608C9EEBAC},19 | Search.Contents | 0 | PDF サンプル#3

`Title`, `Author`, `Subject`, `Keywords` については、空文字列の場合はプロパティを出力しません。

`Search.Contents` については、ページごとにプロパティを 1 つ出力します。これは内容が空であっても出力するため、ページ数の数だけ出力します。

`idChunk` は 1 から連番で付与します。スキップしたプロパティについても増分するため、この属性へ依存するアプリは整合性を保つことができます。

`idChunk` と `idChunkSource` とは、常に同じ値を持ちます。

`locale`, `cwcStartSource`, `cwcLenSource` は 0 で固定です。

`breakType` は `CHUNK_EOS` で固定です。

`flags` について: `Title`, `Author`, `Subject`, `Keywords` の場合は `CHUNK_VALUE` を出力します。他の場合については `CHUNK_TEXT` を出力します。

## ビルド方法

Visual Studio 2022 を使ってビルドします。

以下のフォルダーについて、つぎのサイトからバイナリを入手してアーカイブを展開してください [bblanchon/pdfium-binaries: 📰 Binary distribution of PDFium](https://github.com/bblanchon/pdfium-binaries):

```
pdfium-win-x64
pdfium-win-x86
```
