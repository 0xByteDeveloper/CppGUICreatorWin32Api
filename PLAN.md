# 📐 PLAN — CppGUI Creator: VS2019 Benzeri GUI Tasarımcısı (Saf Win32 API)

| | |
|---|---|
| **Sürüm** | v1.0 — Taslak (onay bekliyor) |
| **Tarih** | 2026-10-21 |
| **Referans veri** | `Config_info.json` v1.0.0 (144 eleman / 8.664 property / 5.785 event / 203 enum) |
| **Girdi dokümanı** | `KULLANICI_ISTEGI.md` (birebir kullanıcı isteği) |
| **Hedef repo** | https://github.com/0xByteDeveloper/CppGUICreatorWin32Api |

---

## 1. Vizyon ve Hedefler

Visual Studio 2019'un Windows Forms tasarımcısını, **hiçbir .NET bileşeni kullanmadan, saf Win32 API + C++** ile birebir yeniden inşa ediyoruz. Tasarımcı kendi bağımsız exe'miz olacak; kaynak kodumuz .NET gibi decompile edilemeyecek (native C++). Tasarım yüzeyinde **gerçek Win32 kontrolleri** barındıracağımız için ekranda gördüğümüz şey zaten gerçek önizleme olacak (WYSIWYG) — ayrıca üretilen proje VS 2019'da derlenip çalıştırılabilecek.

Measurable hedefler:

- **H1** — Tasarımcı %100 Win32/C++ (WinMain + RegisterClass + CreateWindowEx + GDI). Hiçbir CLR/WinForms bağımlılığı yok.
- **H2** — VS2019 düzeni birebir: **sol** Toolbox (+arama kutusu), **orta** tasarım yüzeyi, **sağ üst** dosya gezgini, **sağ alt** Properties penceresi (Properties/Events sekme simgeleri + A-Z ve kategoriye göre sıralama).
- **H3** — Obje ekleme, sürükleme, location/size değiştirme, **iç içe yerleştirme** (Panel/GroupBox/TabPage içine).
- **H4** — Toolbox, Properties ve Events listeleri **elle yazılmayacak**; `Config_info.json` veri setinden beslenecek (elemanlar, browsable property'ler, event'ler, enum değerleri, win32 eşlemeleri).
- **H5** — Nesne eklendiğinde/propery değiştiğinde bağlı projenin **`Hedef.cpp`** dosyası otomatik güncellenecek (marker bölgeleri ile; kullanıcı kodu asla bozulmayacak).
- **H6** — Events sekmesinde örn. `Paint`e tıklandığında: **VS 2019 TopMost öne gelir**, `Hedef.cpp` focus olur (kapalıysa açılır), `PictureBox1_Paint` yoksa eklenir, varsa o satıra scroll edilir.
- **H7** — "▶ Derle & Çalıştır" butonu: MSBuild ile projeyi derler ve ürettiğimiz menüyü (exe) çalıştırır.

---

## 2. MİMARİ KARAR: DLL Injection mı, Bağımsız Pencere mi?

Kullanıcının sorduğu temel soru budur ve cevap net:

### 2.1 Karşılaştırma

| Kriter | DLL Injection (VS'e) | **Standalone Pencere (ÖNERİLEN)** |
|---|---|---|
| "Menümüzü görmek" | Anlamsız: menü zaten bizim exe'nin penceresi; VS'e inject etmenin önizlemeyle ilgisi yok | Tasarım yüzeyi **zaten gerçek kontrollerle canlı önizleme** + "▶ Derle & Çalıştır" ile gerçek exe |
| Kaynak gizleme | Inject edilen DLL yine native, ama VS süreç içinde analiz edilebilir | Tam kontrol: kendi exe'miz, `/GL` derleme, PDB yayımlanmaz |
| Antivirüs / güvenlik | `CreateRemoteThread`/`SetWindowsHookEx` = AV false-positive **garantisi** | Hiçbir şüpheli API kullanılmaz |
| VS güncellemeleri | Her VS update'te kırılabilir (devenv iç yapısı sözleşme değildir) | Bağlılığımız **dosya + resmi COM arayüzü (DTE)** — güncellemelere dayanıklı |
| Kullanıcının istediği akış (TopMost → Hedef.cpp focus → stub ekle/scroll) | Gereksiz karmaşıklıkla çözülür | **EnvDTE (COM) ile tamamı mümkün ve resmi yol** |
| Geliştirme hızı / hata ayıklama | Çok zor | Normal C++ geliştirme akışı |
| VSIX alternatifi (VS içinde menü) | C#/WPF gerekir → ".NET istemiyoruz" hedefiyle çelişir | — |

### 2.2 Karar ✅

> **Bağımsız (standalone) Win32 tasarımcı penceresi + "Bağlı Proje" dosya entegrasyonu. DLL injection KULLANILMAYACAK.**

Nesne eklediğimizde tasarımcı, VS projesinin içindeki `Hedef.cpp`yi düzenler (VS'de açık olsun ya da olmasın); event'e tıkladığımızda DTE üzerinden VS öne getirilip ilgili satıra gidilir. Böylece:

- Proje VS 2019 ile **bağlı kalır** (aynı klasör, aynı dosya).
- "Menümüzü görmek" için: **(a)** tasarım yüzeyi (canlı), **(b)** ▶ Derle & Çalıştır (gerçek exe). İkisi de injection gerektirmez.

---

## 3. Genel Mimari

```text
┌───────────────────────────────┐          ┌─────────────────────────────────┐
│  CppGUICreator.exe (Win32)    │          │  VS 2019 (devenv.exe)           │
│                               │          │                                 │
│  UI Shell (ana pencere)       │          │  Çözüm: HedefProje.sln          │
│   ├─ Toolbox (Config_info)    │  dosya   │   └─ Hedef.vcxproj              │
│   ├─ DesignSurface            │◄────────►│       ├─ Hedef.cpp   ◄─ marker  │
│   ├─ SolutionTree             │ senkron  │       ├─ CppGUIRuntime.h        │
│   ├─ PropertyGrid (Props/Evt) │          │       └─ (Config_info.bin kopya)│
│   ├─ CodeGen                  │  DTE     │                                 │
│   ├─ VSLink (EnvDTE/COM)  ────┼──────────┼─► OpenFile / MoveToLine /       │
│   ├─ Watcher (ReadDirChanges) │          │   Activate / TopMost            │
│   └─ ConfigStore (bin)        │  MSBuild │◄─── derle & çalıştır ───────────┤
└───────────────────────────────┘          └─────────────────────────────────┘
```

**Bileşenler:**

| Modül | Sorumluluk |
|---|---|
| `UI Shell` | Ana pencere, splitter'lar (sol %18 / orta / sağ %28), durum çubuğu |
| `Toolbox` | Arama kutusu + kategorili/ikonlu eleman listesi + sürükleme başlatma |
| `DesignSurface` | Form penceresi, gerçek child kontroller, seçim overlay'i, taşıma/boyutlandırma/nesting |
| `SolutionTree` | Bağlı proje dosyaları TreeView (çift tık → VS'de aç) |
| `PropertyGrid` | Properties/Events sekmeleri, A-Z/kategori sıralama, editörler, açıklama paneli |
| `CodeGen` | `.cgproj` → `Hedef.cpp` üretimi (marker diff-güvenli yazım) |
| `VSLink` | vswhere + EnvDTE (IDispatch) + MSBuild çağrıları |
| `Watcher` | `ReadDirectoryChangesW` ile Hedef.cpp izleme, dış değişiklik birleştirme |
| `ConfigStore` | `Config_info.bin` okuyucusu (toolbox/property/event/enum/win32 verileri) |

---

## 4. Tasarımcı Arayüzü Spesifikasyonu (birebir VS2019)

### 4.1 Ekran Düzeni

```text
+--------------------------------------------------------------------------+
| CppGUI Creator                          Dosya Araçlar Yardım      [_ □ X] |
+------------------+------------------------------------------+------------+
| Toolbox          |            DesignSurface                 | Çözüm      |
| [🔍 arama____ ]  |   ┌────────────────────────────────┐     | Gezgini    |
| ▸ Ortak Denetimler│   │ Form1                  — □ ✕   │     | ▸ HedefProje|
|    Button        |   │                                │     |    Hedef.cpp|
|    TextBox       |   │   [PictureBox1]    [Button1]   │     |    CppGUI...|
|    PictureBox    |   │   ┌─ GroupBox1 ────────────┐   │     |            |
|    Label ...     |   │   │  (RadioButton1)        │   │     +------------+
| ▸ Kapsayıcılar   |   │   │                        │   │     | ⚙|⚡  ⇅|≡   |
| ▸ Menüler        |   │   └────────────────────────┘   │     | Properties |
+------------------+---+────────────────────────────────+────+|────────────|
| Durum: Form1 · PictureBox1 (40,50) 200x120                  | Name  Pic.. |
|                                                             | Text  ""    |
|                                                             | Location 40,50
|                                                             | Paint  (çift tıkla)
+-------------------------------------------------------------+------------+
```

Sağ alt panelin başlığındaki simgeler (VS2019 gibi): **⚙ Properties**, **⚡ Events**, ve sıralama için **A→Z** ile **kategoriye göre** görünüm anahtarları.

### 4.2 Toolbox (Sol)

- Üstte arama kutusu (`EDIT` + `EN_CHANGE`): yazıldıkça liste filtrelenir (isim + TR/EN kategori adı üzerinden).
- Liste: `Config_info.toolbox_layout` kategorileri (Ortak Denetimler, Kapsayıcılar, Menüler ve Araç Çubukları, Veri, Bileşenler, İletişim Kutuları, Yazdırma) — kategori başlıkları açılır/kapanır.
- Satırlar owner-draw: eleman ikonu (16x16, resource) + adı. İkonlar Faz 1'de basit çizim, Faz 6'da VS benzeri.
- Sürükleme: `WM_LBUTTONDOWN` → `SetCapture` → yarı saydam "ghost" çizimi → yüzeyin üzerinde `WM_LBUTTONUP` → bırakılan noktada **nesting kurallarına göre** hata bul (hit-test en üstten) → kontrol oluştur.
- Sadece `toolbox.visible=true` elemanlar listelenir (Config_info verisi).

### 4.3 Design Surface (Orta) — Canlı Önizleme

- Form = **gerçek bir Win32 penceresi**; her kontrol = **gerçek `CreateWindowEx` child'ı** (`win32_mapping.window_class` ile). Yani yüzeydeki görüntü = çalışma zamanındaki görüntü.
- **Seçim overlay'i**: yüzeyi kaplayan şeffaf child pencere; seçili kontrolün etrafına mavi çerçeve + **8 tutamaç** (grip) çizer. Overlay, kontrolün painting'ine karışmaz (VS de böyle yapar).
- **Taşıma**: seçili kontrol + `WM_MOUSEMOVE` → delta ile konum güncelle → `SetWindowPos` → `.cgproj` değişikliği → CodeGen kuyruğu (debounce 400ms).
- **Boyutlandırma**: 8 tutamactan sürükle (köşe/kenar); min boyut 2x2; `SizeGrip` görseli.
- **Nesting (iç içe koyma)**: bırakma anında hit-test → altındaki kontrollerden `child_item` kuralına uyan (Config_info: Panel/GroupBox/TabPage → `Control`; TabControl → `TabPage`...) **`SetParent`** + koordinat dönüşümü (ekran→parent). Zaten parent'ı değiştirilebilir (Drag ile GroupBox'tan Panele taşıma).
- Z-order: `BringWindowToTop` / sağ-tık menüsü ("Öne getir / Arkaya gönder").
- Grid snap (Faz 5), ok tuşlarıyla 1px hareket (Shift=10px), `Del` silme, `Ctrl+D` çoğaltma.
- Redraw: `WS_CLIPCHILDREN` + `InvalidateRect` doğru kapsamla.

### 4.4 Çözüm Gezgini (Sağ Üst)

- `TreeView`: Bağlı Proje kökü → dosyalar (Hedef.cpp, CppGUIRuntime.h, Config_info.bin, .cgproj).
- Çift tık → `VSLink.OpenFile` (VS'de açılır + öne gelir).
- Sağ-tık: "VS'de Aç", "Klasörde Göster", "Yeniden Yükle".

### 4.5 Properties Penceresi (Sağ Alt)

- **İki sekme simgesi**: ⚙ Properties / ⚡ Events.
- **Sıralama anahtarları**: A→Z (alfabetik) / Kategori (grup başlıkları) — VS'deki gibi; Config_info'daki `category` alanından beslenir.
- **Grid**: virtual owner-draw `ListView` (report, satır yüksekliği 20px): sol hücre adı, sağ hücre editör. Kategori modunda gri başlık satırları. En altta **açıklama paneli** (seçili property'nin `description` metni — Config_info'dan).
- **Editör türü seçimi** (property tipine göre otomatik):
  | Tip | Editör |
  |---|---|
  | `System.String` | metin kutusu |
  | `System.Boolean` | checkbox |
  | `enum` (`enum_type` alanı) | dropdown (değerler `enums` bölümünden; örn. DockStyle: None/Top/Bottom/...) |
  | `System.Drawing.Point/Size` | "40, 50" metin + ayrı X/Y, W/H alt satırları |
  | `System.Drawing.Color` | dropdown (Adlandırılmış renkler) + diyalog |
  | `System.Drawing.Font` | "..." butonu → `ChooseFont` |
  | `Image` (PictureBox.Image) | "..." → dosya seç → `LoadImage`/GDI+ (Faz 4) |
- **v1'de gösterilen property'ler**: `browsable=true` olanlar (Config_info'da işaretli) + Name (tasarımcı extender'ı, eklenir). Kalanlar (browsable=false) ileri seviye filtresiyle açılabilir.
- **Events sekmesi**: seçili kontrolün event listesi (browsable olanlar başta). Her satır: Event adı + handler adı (`PictureBox1_Paint`) veya boşsa `(yok)`. **Tek tık → satır düzenleme (handler adı), çift tık → Bölüm 7.3'teki VS akışı.**
- Property değişince: anında yüzeye uygula (`SetWindowPos`/`SetWindowText`/`ShowWindow`...) + `.cgproj` + CodeGen debounce.

### 4.6 Klavye / Kısayollar

`F2` yeniden adlandır · `Del` sil · oklar taşı · `Ctrl+Z/Y` geri al/ileri (Faz 5) · `Ctrl+C/V` kopyala/yapıştır (Faz 5) · `F5` Derle & Çalıştır · `F7` VS'de Hedef.cpp'yi aç.

---

## 5. Veri Katmanı — Config_info Kullanımı

**Ilke: Hiçbir araç/property/event listesi elle yazılmaz.** Tümü `Config_info.json`dan gelir (bu veri seti tam bu amaçla üretildi: 144 eleman, property'lerin VS penceresindeki kategori/açıklama/browsable bilgileri, event'ler, enum değerleri, win32_mapping ve child_item ilişkileri).

- **Build-time**: `tools/cfg2bin` → `Config_info.json` → `Config_info.bin` (özel ikili format: tip tabloları + string havuzu; opsiyonel XOR/AES şifreleme → kaynak gizleme + yükleme hızı). Designer exe gömer.
- **Runtime API** (`ConfigStore`):
  ```cpp
  CfgToolboxItems();                    // toolbox kategorileri + sıra
  CfgElement(name);                     // kind, base, default_event/property, win32_mapping...
  CfgProps(name);                       // name, type, category, description, browsable, enum_type...
  CfgEvents(name);                      // name, handler_type, category...
  CfgEnumValues(enumName);              // enum dropdown'ları
  CfgWin32Class(name);                  // CreateWindowEx sınıf adı + not
  CfgChildItem(name);                   // nesting kuralı (type, collection)
  ```
- **Veri → UI eşlemesi**: Toolbox←toolbox_layout/visible · PropGrid←properties[](+enums) · Events sekmesi←events[] · Nesting←child_item · CodeGen←win32_mapping · Sıralama←category.
- **Faz 0 genişletmesi (Config_info v1.1)**: yeni bölümler eklenecek — `win32_event_map` (event adı → Win32 mesaj/bildirim kodu + subclass gereksinimi, bkz. Bölüm 8) ve `property_style_map` (property adı → Win32 stil bayrağı/seter çağrısı). Böylece CodeGen tamamen veri güdümlü kalır.

---

## 6. Proje Formatı ve Kod Üretimi

### 6.1 Bağlı Proje Klasörü

```text
HedefProje/                     ← VS 2019 C++ projesi (boş Win32 şablonundan)
  Hedef.sln
  Hedef.vcxproj
  Hedef.cpp                     ← Designer ÜRETİR (marker bölgeleri) + KULLANICI kodu
  CppGUIRuntime.h               ← sabit çalışma zamanı (elle yazılır, designer değiştirmez)
  Config_info.bin               ← çalışma zamanı ihtiyacı: kontrol listesi vs. (v2)
  Hedef.cgproj                  ← tasarımcının kendi proje dosyası (JSON) — TEK GERÇEK KAYNAK
```

### 6.2 Tek Gerçek Kaynak ve Marker Bölgeleri

- **`Hedef.cgproj`** = tasarımcının modeli (form + kontroller + property + event-handler bağları).
- **`Hedef.cpp`** = üretilen çıktı **+** kullanıcı event kodu. Designer yalnızca şu işaretli bölgeleri yazar:

```cpp
//[GUI:BEGIN DECL]   otomatik — elle düzenlemeyin
...
//[GUI:END DECL]

//[GUI:BEGIN CONTROLS]
...
//[GUI:END CONTROLS]

//[GUI:BEGIN EVENTS]
...
//[GUI:END EVENTS]

// (kalan her şey KULLANICI koda aittir — designer asla dokunmaz)
```

- Yazma algoritması: dosyayı oku → marker dışı bölümleri **birebir koru** → marker içlerini yeniden üret → yaz (UTF-8 **BOM** + CRLF). Yazmadan önce `Hedef.cpp.bak`. Böylece kullanıcı VS'de yaptığı düzenlemeleri kaybetmez.

### 6.3 Üretilen Kod Örneği (PictureBox, GroupBox içinde + Paint hookup)

```cpp
// Hedef.cpp — CppGUI Creator (Win32) tarafından yönetilir
#include <windows.h>
#include "CppGUIRuntime.h"

//[GUI:BEGIN DECL]
#define IDC_FORM1        100
#define IDC_GROUPBOX1    101
#define IDC_PICTUREBOX1  102
static HWND  g_hForm1, g_hGroupBox1, g_hPictureBox1;
static HFONT g_hFontUI;
//[GUI:END DECL]

// ==== KULLANICI EVENT KODU (stub'ı designer ilk seferde ekler; gövde kullanıcıya aittir) ====
void PictureBox1_Paint(HWND hCtl, HDC hdc)
{
    // TODO: çizim kodunuzu buraya yazın
}

//[GUI:BEGIN CONTROLS]
static void CGGUI_CreateControls(HWND hParent, HINSTANCE hInst)
{
    g_hGroupBox1 = CreateWindowExW(0, L"BUTTON", L"Grup",
        WS_CHILD | WS_VISIBLE | WS_GROUP | BS_GROUPBOX,
        24, 24, 280, 180, hParent, (HMENU)IDC_GROUPBOX1, hInst, NULL);
    CGGUI_SetFont(g_hGroupBox1, g_hFontUI);
    CGGUI_Subclass(g_hGroupBox1, IDC_GROUPBOX1);

    g_hPictureBox1 = CreateWindowExW(0, L"STATIC", L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | SS_NOTIFY | SS_BITMAP,
        16, 26, 200, 120, g_hGroupBox1, (HMENU)IDC_PICTUREBOX1, hInst, NULL);
    CGGUI_SetFont(g_hPictureBox1, g_hFontUI);
    CGGUI_Subclass(g_hPictureBox1, IDC_PICTUREBOX1);
}
//[GUI:END CONTROLS]

//[GUI:BEGIN EVENTS]
static const CGGUI_EventMap CGGUI_Events[] = {
    { IDC_PICTUREBOX1, CGGUI_EV_PAINT, (CGGUI_Handler)PictureBox1_Paint },
};
static const int CGGUI_EventCount = 1;
//[GUI:END EVENTS]
```

> Koordinat notu: PictureBox **GroupBox içinde** olduğu için üretim, parent'ına göre **göreli** koordinat yazar (24,24 yerine 16,26) — nesting'in doğal Win32 karşılığı.

### 6.4 `CppGUIRuntime.h` (sabit çalışma zamanı)

- `CGGUI_Subclass(hWnd, id)`: `SetWindowSubclass` ile her kontrole mini dispatcher; `WM_PAINT/WM_MOUSEMOVE/WM_KEYDOWN/...` yakalayıp event map'te karşılığı varsa kullanıcı fonksiyonuna iletir, aksi halde orijinal proc'a bırakır.
- `WM_COMMAND`/`WM_NOTIFY` yansıması (reflect): ana pencere proc'u, bildirimleri sahibi kontrolün subclass'ına yönlendirir.
- `CGGUI_SetFont`, `CGGUI_SetBackColor` (WM_CTLCOLOR brush cache), `CGGUI_Layout` (Anchor/Dock motoru, `WM_SIZE`'da).
- Event map tablosu sayesinde **kullanıcı .cpp'ye hiçbir altyapı kodu yazmaz**.

### 6.5 Kod Üretim Kuralları

- İsimlendirme VS tarzı: `PictureBox1`, `PictureBox2` (Config_info `name` + sayaç; Türkçe karakter ASCII'ye çevrilir).
- Property → kod: `Location/Size` → `CreateWindowEx` koordinatları; `Text` → başlık; `Visible/Enabled/BorderStyle/TabStop` → stil bayrakları; `Font` → `CGGUI_SetFont`; `BackColor` → `CGGUI_SetBackColor`; `Items` → `LB_ADDSTRING` serisi; `Image` → `LoadImageW` satırı (v2 resource'a taşınır).
- Event → kod: stub fonksiyon + `CGGUI_Events` tablosuna kayıt (Bölüm 8 eşlemesine göre).

---

## 7. VS 2019 Entegrasyonu (`VSLink` modülü)

### 7.1 VS'yi Bulma

```
"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" ^
  -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 ^
  -property installationPath
→ devenv.exe : <path>\Common7\IDE\devenv.exe
→ MSBuild    : <path>\MSBuild\Current\Bin\MSBuild.exe
```

Yedek: registry (`HKLM\SOFTWARE\Microsoft\VisualStudio\SxS\VS7` 16.0).

### 7.2 DTE Köprüsü (saf C++ COM — .NET YOK)

- `CLSIDFromProgID(L"VisualStudio.DTE.16.0")` → `GetActiveObject` (ROT) → `IDispatch*`. (VS2022 desteği için 17.0 fallback.)
- **Late binding**: tüm çağrılar `IDispatch::GetIDsOfNames`/`Invoke` ile — compile-time bağımlılık yok.
- Çoklu VS örneği: ROT'tan tüm DTE'ler taranır; `Solution.FullName` eşleşen seçilir.
- Temel işlemler (hepsi IDispatch zinciri):

| İşlem | DTE çağrısı |
|---|---|
| Dosya aç | `DTE.ItemOperations.OpenFile(path)` |
| Satıra git | `doc.Object("TextDocument").Selection.MoveToLineAndOffset(line, 1, false)` |
| Öne getir (TopMost) | `DTE.MainWindow.Activate()` + `SetForegroundWindow(DTE.MainWindow.HWnd)` |
| Çözüm açık mı? | `DTE.Solution.FullName` |
| Kapalıysa aç | `CreateProcess(devenv.exe Hedef.sln)` → DTE retry döngüsü (60sn) |

### 7.3 ⚡ Event Tıklama Akışı (kullanıcı isteğinin birebir karşılığı)

1. Kullanıcı Events sekmesinde `Paint` satırına **çift tıklar** (seçili: `PictureBox1`).
2. Designer, `Hedef.cpp` içinde `PictureBox1_Paint(` arar (regex tarama).
3. **YOKSA**: (a) stub fonksiyonu kullanıcı bölgesine eklenir, (b) `CGGUI_Events` tablosuna kayıt eklenir (EVENTS marker'ı), (c) dosya yazılır (marker-güvenli + .bak), (d) stub'ın satır numarası hesaplanır.
4. `VSLink`: VS açık mı? → değilse `devenv Hedef.sln` başlat + DTE hazır olmasını bekle → `OpenFile(Hedef.cpp)` → `MoveToLineAndOffset(stubSatırı)` → `MainWindow.Activate()` + `SetForegroundWindow` → **VS TopMost, Hedef.cpp focused, imleç stub üzerinde**.
5. **VARSA**: kod eklenmez; sadece adım 4'ün focus+scroll kısmı çalışır.

### 7.4 Dosya İzleme ve Çakışma Yönetimi

- `ReadDirectoryChangesW` (proje klasörü, `FILE_NOTIFY_CHANGE_LAST_WRITE`).
- VS tarafında bekleyen kaydedilmemiş değişiklik varsa yazmadan önce `DTE.Document.Save()` denenir → sonra oku → birleştir.
- Dış değişiklik yalnızca marker dışıysa: `.cgproj`'e dokunma, sadece durum çubuğunda bilgi.
- VS, dış değişiklikte açık belgeyi otomatik yeniden yükler (VS 2019 varsayılanı "Detect Changes Automatically" açık).

### 7.5 ▶ Derle & Çalıştır

```
MSBuild.exe Hedef.vcxproj /m /v:m /p:Configuration=Debug /p:Platform=x64
→ %ERRORLEVEL% 0 ise: exe'yi CreateProcess ile başlat
→ hata satırları designer'ın "Çıktı" paneline (dockable alt panel, Faz 5)
```

---

## 8. Property → Win32 ve Event → Win32 Eşlemeleri

### 8.1 Event → Win32 Mesaj (v1 çekirdek küme — Config_info v1.1'e `win32_event_map` olarak girer)

| Event | Win32 kaynağı | Yol |
|---|---|---|
| Paint | `WM_PAINT` | subclass |
| Click | `WM_COMMAND` (BN_CLICKED/STN_CLICKED) | reflect |
| DoubleClick | `WM_LBUTTONDBLCLK` | subclass |
| MouseDown/Up/Move/Enter/Leave | `WM_LBUTTONDOWN/UP`, `WM_MOUSEMOVE`, `WM_MOUSELEAVE` | subclass |
| KeyDown/KeyUp/KeyPress | `WM_KEYDOWN/UP`, `WM_CHAR` | subclass |
| TextChanged | `EN_CHANGE` | reflect |
| CheckedChanged | `WM_COMMAND` (BN_CLICKED) | reflect |
| SelectedIndexChanged | `LBN_SELCHANGE`, `CBN_SELCHANGE`, `LVN_ITEMCHANGED` | reflect |
| Scroll | `WM_HSCROLL`/`WM_VSCROLL` | reflect |
| Tick (Timer) | `WM_TIMER` | runtime |
| Load (Form) | `WM_CREATE` sonrası ilk `WM_SHOWWINDOW` | runtime |
| FormClosing (Form) | `WM_CLOSE` (Veto: `EndDialog`/flag) | runtime |
| Resize (Form) | `WM_SIZE` | runtime |
| GotFocus/LostFocus | `WM_SETFOCUS`/`WM_KILLFOCUS` | subclass |

Kalan event'ler (DragDrop, HelpRequested, Validating...) v2'de eşlenir; UI'da "gelişmiş" filtresiyle görünür.

### 8.2 Property → Win32 (v1)

| Property | Win32 karşılığı |
|---|---|
| Location, Size | `CreateWindowEx` x/y/cx/cy (+parent'a göre göreli) |
| Text | `SetWindowTextW` / `CreateWindowEx` başlık |
| Visible / Enabled | `WS_VISIBLE` / `WS_DISABLED` |
| BorderStyle | `WS_BORDER` / `WS_EX_CLIENTEDGE` |
| TabStop, TabIndex | `WS_TABSTOP` + z-order |
| Font | `WM_SETFONT` (HFONT cache) |
| BackColor / ForeColor | `WM_CTLCOLOR*` + HBRUSH cache |
| Items (ListBox/ComboBox) | `LB_ADDSTRING` / `CB_ADDSTRING` serisi |
| Image (PictureBox) | `STM_SETIMAGE` (HBITMAP) |
| Checked (CheckBox/Radio) | `BM_SETCHECK` |
| Value/Min/Max | `PBM_SETRANGE32`, `TBM_*`, `UDM_*` |
| Anchor / Dock | `CGGUI_Layout` runtime motoru (WM_SIZE yeniden hesap) |

---

## 9. Kaynak Gizleme Stratejisi

- Native C++ → .NET IL decompilation yok.
- `Config_info.json` repoda açık kalır (geliştirme kaynağı) ama dağıtımda **`Config_info.bin`** (özel format + opsiyonel şifreleme) gömülür.
- Derleme: `/GL` (LTO), PDB yayımlanmaz; string tablosu Faz 6'da opsiyonel şifrelenebilir.
- Injection/RE araçlarına karşı ekstra bir savunma hedeflenmez (boyut/performans dengesi) — yeterince koyu (opaque) native kod.

---

## 10. Faz Planı ve Kabul Kriterleri

| Faz | İçerik | Kabul Kriteri |
|---|---|---|
| **0 — Veri v1.1** | `Config_info`'ya `win32_event_map` + `property_style_map` bölümleri | Her toolbox elemanının çekirdek event'leri eşlenmiş; cfg2bin çalışıyor |
| **1 — Tasarımcı İskeleti** | Ana pencere, splitter'lar, Toolbox(+arama), DesignSurface: ekle/taşı/boyutlandır/seç/nesting, SolutionTree, Properties penceresi (salt-okunur grid + sekmeler + sıralama anahtarları) | 5 farklı kontrol ekle; birini Panel içine yerleştir; taşı, boyutlandır; seçim tutamaçları çalışıyor |
| **2 — Proje + Kod Üretimi** | `.cgproj` kaydet/yükle, marker-güvenli `Hedef.cpp` üretimi, `CppGUIRuntime.h` v0, VS boş proje şablonu | Üretilen proje VS 2019'da **derlenip çalışıyor**; GroupBox içindeki PictureBox'un Paint stub'ı gerçekten tetikleniyor |
| **3 — VS Entegrasyonu** | vswhere, DTE (IDispatch), event tıklama akışı (7.3), Watcher, ▶ Derle & Çalıştır | `Paint`e çift tık → VS TopMost + Hedef.cpp focused + stub ekle **veya** scroll; VS kapalıyken de çalışıyor |
| **4 — Properties/Events Tam** | Tüm editör türleri (enum/renk/font/image), açıklama paneli, A-Z/kategori sıralama, Name yönetimi | VS özellik penceresi deneyimi: her browsable property düzenlenebilir ve koda yansır |
| **5 — Üretkenlik** | Undo/Redo, pano, grid snap, Anchor/Dock motoru, Çıktı paneli, F5 | F5: derle→çalıştır; anchor'lı kontrol pencere boyutlanınca doğru hizalanıyor |
| **6 — Cila** | VS benzeri ikonlar, DPI (PerMonitorV2), multi-select (v2), gelişmiş event eşlemeleri, string şifreleme | Görsel olarak VS2019'a yakın; 4K'da nettir |

**Tahmini hacim**: CppGUIRuntime ~1.5K satır · CodeGen ~800 · VSLink ~400 · UI ~3.5K · ConfigStore ~600.

---

## 11. Riskler ve Önlemler

| Risk | Önlem |
|---|---|
| AV false-positive (native exe) | Injection-benzeri API yok; gerekirse kod imzalama |
| DTE bulunamıyor (VS farklı/yönetici oturumu) | Fallback: `devenv /edit "Hedef.cpp"` (focus+scroll yok) + kullanıcıya uyarı |
| VS'de kaydedilmemiş Hedef.cpp değişikliği | Yazmadan önce `Document.Save()` denemesi + .bak + birleştirme |
| Marker dışı elle düzenlemeler | Diff-koruma (marker dışı birebir korunur) + durum çubuğu uyarısı |
| Türkçe karakterler (isimler) | Kontrol adları ASCII'ye normalize; metinler Unicode (W API) |
| DPI/ölçek | PerMonitorV2 manifest; Font ölçüleri device-independent hesap |
| Boşluklu/uzun yollar | Tüm komutlar tırnaklı; `\\?\` gerekirse |
| Aynı anda iki designer örneği | Tek-örnek kilit (mutex) + uyarı |

---

## 12. Onay Bekleyen Kararlar (açık sorular)

1. **Üretilen uygulama pencere tipi**: düz Win32 pencere (öneri ✅) mı, dialog-based mi?
2. **Platform**: x64 (öneri ✅) + ek olarak Win32?
3. **C++ standardı**: C++17 (öneri ✅)?
4. **v1 Form sayısı**: tek Form (öneri ✅), çoklu v2?
5. **Proje dosyası uzantısı**: `.cgproj` uygun mu?
6. **Stub imzası**: `void PictureBox1_Paint(HWND hCtl, HDC hdc)` (öneri ✅)?
7. **Faz 0**: `Config_info` v1.1'i (event/property eşleme bölümlerini) şimdi üretip pushlayayım mı?

> Onayınızın ardından **Faz 0 → Faz 1** sırasıyla başlıyoruz.
