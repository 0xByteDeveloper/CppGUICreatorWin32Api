# CppGUICreatorWin32Api

**Win32 API ile saf C++ görsel GUI tasarımcısı** — Visual Studio 2019 Windows Forms deneyiminin .NET'siz, native yeniden inşası.

> Pure C++ / Win32 API visual GUI creator. No CLR, no .NET — a native `CppGUICreatorWin32Api.exe`.

---

## 🚀 Uygulama (v1.0)

### Ekran düzeni (kullanıcı isteğine birebir)

```text
+--------------------------------------------------------------------------+
| CppGUI Creator          [Welcome][Projeler][Araç Kutusu][Görünüm]        |
|                          [Bağlantı][Yardım]        (● devenv: 2)         |   ← menüitem seçici
| [Button][Label][TextBox][CheckBox]...                        (＋)         |   ← seçenekler, "+" instance
+--------------------------------------------------------------------------+
| [Welcome Page ×]  [MyForm ×]  [Adsız1 ×]                                  |   ← TabControl (kapatılabilir)
+--------------------------------------------------------------------------+
|  Sekme içeriği: Welcome Page  /  Form tasarımcısı + Özellikler/Olaylar    |
+--------------------------------------------------------------------------+
| Durum: pictureBox1_Paint oluşturuldu — satır 42                           |   ← durum çubuğu
+--------------------------------------------------------------------------+
```

* **Welcome Page** (ilk sekme): projeler **en yeni önce** listelenir, **"＋ Yeni Proje"** kartı ve çalışan `devenv.exe` bağlantı çipleri gösterilir.
* **Sekmeler kapatılabilir** (`×`); Welcome Page üst menüden geri açılır.
* **"+" işareti**: seçili kontrol tipinden **instance oluşturur** (Araç Kutusu menüsünde) veya **yeni proje** açar (Projeler menüsünde).
* **Tema**: Apple tarzı sade modern — `#F5F5F7` zemin, `#0071E3` aksan, pill butonlar, yumuşak gölgeler, GDI+ + Segoe UI.

### devenv.exe keşfi (x1 / x2 ortak kaynak)

* `Toolhelp32Snapshot` ile çalışan **devenv.exe** süreçleri taranır.
* Ana pencere başlığından **çözüm/proje adı** ayrıştırılır; PID, exe yolu, başlama zamanı okunur.
* `source\repos`, `Documents\Visual Studio 2019\Projects` vb. köklerdeki `.sln`/`.vcxproj`/`.cpp` ile zenginleştirilir.
* Bu projeler **Projeler menüsünde**, **Bağlantı menüsünde** ve **Welcome Page**'de seçenek olarak listelenir.

### VS2019 bağlantısı ve olay kodu üretimi

| Eylem | Sonuç |
|---|---|
| Kontrolere **çift tık** | Varsayılan olayın handler'ı bağlı `.cpp` içinde aranır: **varsa** VS açılır + o satıra gidilir; **yoksa** dosya sonuna stub eklenir + satıra gidilir |
| **Forma çift tık** | `<Proje>_Load` üretilir (örn. `MyForm_Load`) |
| VS2019 her zaman **öne gelir** | DTE (`OpenFile` + `MoveToLineAndOffset`) → bulunamazsa `ShellExecute` yedeği |
| İsimlendirme | `pictureBox1_Paint`, `MyForm_Load` (VS tarzı; Türkçe karakterler ASCII'ye normalize) |

#### Win32 sender / e karşılıkları (CLR → Win32)

```cpp
// CLR (eski):  private: System::Void MyForm_Load(System::Object^ sender, System::EventArgs^ e)
// Win32 (yeni): tam karşılık — .NET yok, inceleyip kontrol edebilirsiniz:
void MyForm_Load(W32Object* sender, W32EventArgs* e)
{
    // sender->TypeName / sender->Name / sender->Hwnd / sender->Id
    // e->Message / e->WParam / e->LParam / e->X / e->Y / e->KeyCode
    // e->Handled = true;   // varsayılan işlemi durdur (kontrol edilebilir e)
}
```

Tipler `runtime/W32GuiRuntime.h` içinde tanımlıdır (header-only, saf Win32). Handler üretilirken dosyaya `#include "W32GuiRuntime.h"` eklenir ve header proje klasörüne kopyalanır (mevcut kullanıcı kodu **asla** bozulmaz — `.bak` yedek alınır).

### Form tasarımcısı

* Seçim + 8 tutamaç, sürükleme, SE/E/S ile boyutlandırma, 8px ızgara yapışma, `Del` ile silme.
* **Özellikler** paneli: Name / Text / X / Y / W / H + Config_info'dan gelen browsable property'ler (satıra tıkla → düzenle).
* **Olaylar** paneli: Config_info'daki event listesi — **çift tık → VS2019 akışı**.
* Kontroller Config_info.json'un `win32_mapping` ipuçlarıyla Apple-temiz çizimle render edilir (Button, Label, TextBox, CheckBox, RadioButton, GroupBox, PictureBox, ComboBox, ListBox, ProgressBar, TrackBar, DateTimePicker, Panel, MenuStrip + jenerik placeholder).
* Düzen otomatik kaydedilir: `%LOCALAPPDATA%\CppGUICreatorWin32Api\layouts\<proje>.layout`.
* **Eski projeler**: depolanmış düzen → `assets/default_<Proje>.layout` (örnek MyForm) → boş form sırasıyla yüklenir.

### Projeyi Aktar (gerçek Win32 uygulaması üretir)

`Yardım → "Aktif projeyi aktar"`: tasarımdaki formu `<Proje>_w32.cpp` + `W32GuiRuntime.h` + `build_mingw.bat` olarak bağlı proje klasörüne (veya `Masaüstü\CppGUIExport\<Proje>`) yazar. Üretilen uygulama gerçek bir Win32 penceresi açar, kontrolleri oluşturur ve olay tablonuzu dispatch eder. *(Bu repo'daki CI benzeri doğrulama: `tests/export_test.sh` — üretilen kaynak mingw-w64 ile derlendi ✓)*

---

## 🔨 Derleme

| Yol | Komut | Not |
|---|---|---|
| **VS2019** | `build_vs2019.bat` (veya `.sln`'i aç, F5) | v142 toolset; **Windows 10 SDK** yeterli — VS2019 SDK gerekmez |
| **MinGW** | `build_mingw.bat` | `x86_64-w64-mingw32-g++` veya `g++` (MinGW) |
| **Linux cross** | `MINGW_CXX=... ./build.sh` | llvm-mingw / mingw-w64 cross derleme |
| **CMake** | `cmake -B build && cmake --build build --config Release` | MSVC/MinGW |

Gereken kütüphaneler (hepsi Windows SDK ile gelir): `user32 gdi32 gdiplus comctl32 comdlg32 shell32 advapi32 ole32 oleaut32 uuid shlwapi`.

### Bağımlılık Raporu (uygulama içi: Yardım → Bağımlılık Raporu)

* DTE erişimi **IDispatch late binding** iledir — compile-time VS SDK bağımlılığı **yoktur**.
* VS bulunamazsa: `.cpp` `ShellExecute` ile açılır, handler yine dosyaya yazılır, satır no durum çubuğunda gösterilir.
* Konfigürasyon: `assets/catalog.cfg` — `Config_info.json`'dan `scripts/bake_catalog.py` ile üretilir (38 araç kutusu elemanı / 608 property / 684 event).

---

## 🧪 Testler

```bash
tests/run.sh          # çekirdek birim testleri (serialization, CodeGen, isimlendirme)
tests/export_test.sh  # uçtan uca: layout → üretilen Win32 app → cross-derleme
```

| Sınama | Sonuç |
|---|---|
| Birim testleri (36 assertion) | ✅ ALL PASSED |
| Ana uygulama cross-derleme (llvm-mingw, x64) | ✅ `CppGUICreatorWin32Api.exe` (PE32+ GUI) |
| Üretilen standalone uygulama derlemesi | ✅ `MyForm_w32.exe` |
| VS2019 üzerinde gerçek zamanlı test | ⏳ Windows + VS2019 gerektirir (geliştirme ortamı Linux) |

---

## 📚 Belgeler

| Doküman | İçerik |
|---|---|
| [KULLANICI_ISTEGI.md](KULLANICI_ISTEGI.md) | Projenin kurucu vizyonu — birebir kullanıcı isteği |
| [PLAN.md](PLAN.md) | Mimari plan: injection↔standalone kararı, tasarımcı UI spec, kod üretimi, VS 2019 entegrasyonu (EnvDTE), faz planı |

## Config_info.json

**Visual Studio 2019 C++/CLR (Windows Forms) tüm eleman kataloğu** — Win32 API GUI üreticisi için yapılandırma kaynağı.

Veri kaynağı: Microsoft .NET Framework **4.8 reference assemblies** → `System.Windows.Forms.dll` (v4.8.4250.0), `System.Reflection.MetadataLoadContext` ile gerçek reflection; kategori/açıklama metinleri resmi SR resource tablosundan çözülmüştür.

### İçerik

| Veri | Adet |
|---|---|
| Eleman (element) | 144 |
| Property (kalıtım dahil, FLAT) | 8.664 |
| Event | 5.785 |
| Enum (değerleriyle) | 203 |

Kapsam: Form, tüm kontroller, kapsayıcılar, menü/araç çubukları, veri bileşenleri, diyaloglar, yazdırma bileşenleri, ToolStripItem ailesi, DataGridView kolon/hücreleri, TreeNode/ListViewItem öğeleri ve .NET 1.1 legacy kontrolleri (DataGrid, MainMenu, StatusBar, ToolBar, Splitter...).

### Yapı

```text
meta            → sürüm, kaynak, sayaçlar, notlar
toolbox_layout  → VS 2019 toolbox kategorileri (EN + TR) → eleman listeleri
elements{}      → tip başına tüm metadata
enums{}         → enum adı → namespace + değerler (+ flags)
```

`elements` altındaki her tip:

| Alan | Açıklama |
|---|---|
| `name` / `namespace` / `base` | Tip kimliği ve kalıtım zinciri |
| `kind` | form, container, control, dialog, component, menu_item, grid_column, grid_cell, tree_node, list_item, legacy_* ... |
| `toolbox` | VS toolbox görünürlüğü + kategori (EN/TR) |
| `default_event` / `default_property` | VS'de kalın (bold) gösterilen varsayılanlar |
| `child_item` | Tasarımcıda barındırdığı çocuk tipi ve koleksiyon (örn. TabControl → TabPages: TabPage) |
| `win32_mapping` | Win32 API eşleme ipucu: `window_class` + `note` (BUTTON, EDIT, STATIC, SysListView32...) |
| `properties[]` | `name, type, category, description, browsable, can_set, designer_readonly, default, enum_type, localizable, declared_by` |
| `events[]` | `name, handler_type, category, description, browsable, declared_by` |

Notlar:

- `properties` / `events` listeleri **FLAT**'tir: kalıtımla gelen tüm üyeler dahildir (VS özellik penceresi görünümü). Yalnızca tipe özgü üyeler için `declared_by` filtreleyin.
- `browsable=false` üyeler VS Properties penceresinde görünmez; veri bütünlüğü için dahil edildi.
- VS özellik penceresindeki `Name` alanı tasarımcı extender'idır (reflection'da yoktur); her elemana tasarımda benzersiz bir `Name` verilir.
- `win32_mapping` birebir karşılık değil, üretici için öneridir.

### Örnek

```json
"Button": {
  "kind": "control",
  "base": "ButtonBase",
  "default_event": "Click",
  "default_property": "Text",
  "toolbox": { "visible": true, "category_en": "Common Controls", "category_tr": "Ortak Denetimler" },
  "win32_mapping": { "window_class": "BUTTON", "note": "BS_PUSHBUTTON / BS_DEFPUSHBUTTON, BS_BITMAP" },
  "properties": [
    { "Text": "System.String", "category": "Appearance",
      "description": "The text associated with the control.",
      "browsable": true, "can_set": true, "declared_by": "Control" }
  ],
  "events": [
    { "name": "Paint", "handler_type": "System.Windows.Forms.PaintEventHandler",
      "category": "Appearance", "browsable": true, "declared_by": "Control" }
  ]
}
```

### Hızlı sorgular (jq)

```bash
jq '.elements | keys' Config_info.json
jq '.elements.PictureBox.events[] | select(.name=="Paint")' Config_info.json
jq '.elements.Button.properties[] | select(.browsable==true) | .name' Config_info.json
jq '.enums.DockStyle.values' Config_info.json
```
