# M2GodWin32 — Faz 2.6.1 God Mode MVP

**Visual Studio 2019 C++ CLR Form Designer deneyimini Win32 API ile yaşatan, tam kapsamlı görsel UI tasarım ortamı.**

> Bu proje `CppGUICreatorWin32Api` deposunun **Faz 2.6.1** geliştirme aşamasıdır.
> Katalog kaynağı olarak depodaki **Config_info.json** (.NET Framework WinForms tam eleman kataloğu: 144 eleman, tüm property'ler + event'ler + 203 enum) kullanılır.


---

## Faz 2.6.1 — Kritik düzeltmeler: ESC zinciri + model yükleme çift-sayfa bug'ı + panoda sayfa taşıma

Kapsamlı denetim sırasında yakalanan ve **üretilen kodu bozabilen** üç kusur giderildi:

1. **ESC artık gerçekten VS gibi çalışır (her odakta).** Hızlandırıcı tablosu `VK_ESCAPE`'i
   `CMD_ESC_CANCEL`'a taşıdığı için yüzeyin kendi `WM_KEYDOWN`'ı Escape'i hiç görmüyordu —
   yalnızca yerleştirme iptali çalışıyordu. Yeni `surface::EscapeChain()` zinciri:
   **sürükleme iptali → yerleştirme iptali → seçimi ÜST KONTROLE yürüt (sayfa→TabControl,
   iç kontrol→GroupBox/Panel) → seçimi kaldır (form)**. Sayfa seçiliyken ESC artık
   TabControl'ü seçer; akıllı ok ve tutamaçlar geri gelir.

2. **KRİTİK: model yükleme çift-sayfa bug'ı.** `Model::FromJson` içindeki `Add(type, 20, 20)`
   çağrısı `autoPages=true` ile çalışıyordu: İptal (koleksiyon düzenleyici), Aç, Geri Al/Yinele
   ve VS'den bağlanma yükleme yollarının HEPSİNDE TabControl'e otomatik tabPage1+tabPage2
   ekleniyor, JSON'daki gerçek sayfalarla BİRLİKTE **yetim sayfa Ctrl'leri** bırakıyordu.
   Sonuç: üretilen kodda kopya `TabPages->Add`, kopya `M2God::TabPage*` bildirimleri ve
   **enum çift tanımı (derleme hatası)**. Düzeltme: FromJson artık autoPages oluşturmaz;
   eski format (Text satırlı) dosyalar mevcut MİGRASYON bloğuyla aynen desteklenir.
   Birim testi: `Add(2 sayfa) → ToJson → FromJson → 3 ctrl (1 tab + 2 sayfa)` OK.

3. **Kopyala/Kes/Yapıştır artık sayfaları da taşır.** TabControl kopyalanıp yapıştırıldığında
   sayfaları (Text/BackColor/Enabled/ToolTip + geometri) yeni kontrole eklenir (VS davranışı).

**Doğrulama:** tasarım akışı (yerleştir → sayfa seç → ESC ile üst seçim → akıllı ok → koleksiyon
düzenleyici → Ekle → İptal geri alması → Kod Üret) uçtan uca çalışır; üretilen proje
**C++14 ve C++17 ile 0 hata**; üretilen uygulama wine altında TabControl sayfa geçişiyle canlı
çalışır (EkranGoruntuleri_Faz2.6.1/).

## Faz 2.6 — TabControl/TabPage koleksiyonu (VS davranışı) + tam kapsam emisyonu

TabControl artık VS CLR Designer gibi çalışır:

**Tasarım zamanı:**
- **Sekme şeridine tıkla → o tabPage SEÇİLİR** — özellik paneli sayfa özelliklerini (Name, Text, BackColor, Font, ToolTipText, Tag...) gösterir; boyutlandırma tutamağı yoktur (sayfa boyutu sekme alanına bağlıdır, VS kuralı).
- **Seçili TabControl'de sağ üstte AKILLI OK (▶)** — tıklayınca menü: **Sekme Ekle / Sekme Kaldır / Sekmeleri Düzenle...**
- **Özellikler panelinde TabPages satırı** — tıklayınca **TabPage Koleksiyon Düzenleyicisi** açılır: üye listesi + Ekle/Kaldır/Yukarı/Aşağı + Ad/Metin/Arka Plan (renk seçici)/Etkin/Araç İpucu düzenleme; İptal → açılış anındaki modele döner.
- **Sayfalar GERÇEK kapsayıcıdır**: bir düğmeyi sayfa alanına bırakın → o sayfanın çocuğu olur (Location sayfa-göreli); sekme değişince çocuklar gizlenir/gösterilir.
- TabControl eklendiğinde otomatik **tabPage1 + tabPage2** oluşur (VS gibi). Eski tasarımlar (Text satırlarına yazılı sayfalar) açılışta otomatik MİGRE edilir; ilk sayfa çocuklarını devralır.

**Üretilen kod / çalışma zamanı (Global.h v2600):**

```cpp
// Tasarımcının ürettiği satırlar:
tabControl1 = new M2God::TabControl(hWnd, IDC_TABCONTROL1, L"", 152, 120, 220, 150);
tabPage1    = tabControl1->TabPages->Add(L"Ayarlar", ID_TABPAGE1);
button1     = new M2God::Button(tabPage1->Handle, ID_BUTTON1, L"Kaydet", 96, 48, 100, 32);

// Kullanıcı kodunda WinForms sözdizimi:
M2God::TabPage* yeni = tabControl1->TabPages->Add(L"Yeni sayfa");   // TabPage* döner
yeni->Text = L"Bilgi";                       // sekme başlığı da güncellenir
tabControl1->TabPages->RemoveAt(2);          // Remove(p) / Clear() / Insert(i, t)
int n = tabControl1->TabPages->Count;
M2God::TabPage* p = tabControl1->TabPages[0];
tabControl1->TabPages->SelectedIndex = 1;    // sayfa geçişi otomatik (göster/gizle)
M2God::TabPage* aktif = tabControl1->SelectedTab();
tabPage1->BackColor = RGB(240, 240, 255);    // sayfa zemini boyanır
```

- Sekme değişiminde **sayfa geçişi otomatiktir** (`__SelChanged`); kullanıcı `SelectedIndexChanged` yazmasa bile doğru sayfa görünür.
- `TabPages->Add(title, id)` tasarımcdan `ID_` makrosuyla çağrılır; elle çağrıda id verilmezse otomatik atanır.
- Sayfaların `Click` olayı çalışır (sayfa penceresi WM_COMMAND üretir); sayfa içi kontrollerin bildirimleri sayfa penceresinden forma köprülenir (ContainerBridge karşılığı).

**Tam kapsam emisyonu (Config_info.json ile %100 hizalama):**
- Her kontrolün **Tag** özelliği artık gerçekten çalışır ve koda yazılır.
- Yeni gerçek özellikler: TextBox `BorderStyle`/`ScrollBars`, CheckBox `ThreeState`/`CheckState`/`Appearance`, Button/CheckBox `FlatStyle`, ListView `Activation`/`HotTracking`/`HoverSelection`, TreeView `ItemHeight`/`LineColor`/`Indent`, ProgressBar `MarqueeAnimationSpeed`, RichTextBox `WordWrap`/`DetectUrls`/`ZoomFactor`, PictureBox `BorderStyle`, DateTimePicker `MinDate`/`MaxDate`, TrackBar `TickStyle`.
- Katalogdaki **tüm** browsable+can_set özellikler panelde görünür; Win32'de doğrudan karşılığı olmayanlar ve varsayılandan sapanlar üretilen koda **yorum satırı olarak** taşınır (hiçbir tasarım bilgisi sessizce kaybolmaz).

**Doğrulama:** Üretilen proje C++14 **ve** C++17 ile 0 hata; runtime `TabPages->Add/RemoveAt/Count/[]/Text` wine altında canlı kanıtlandı (3. sekme eklendi, başlıklar değişti, sayfa geçişinde içerik doğru gösterildi). Tasarımcıda akıllı ok menüsü, koleksiyon editörü ve sayfa seçimi ekran görüntüleriyle doğrulandı (EkranGoruntuleri_Faz2.6/).

---

## Faz 2.5.2 — PictureBox GDI+ resim desteği (Global.h v2502)

İki düzeltme: (1) v2500–v2501'de PictureBox kodundaki **derleme hatası** (`PicTable()]`) giderildi — PictureBox içeren hiçbir proje v2501 Global.h ile derlenemiyordu; (2) `Image` artık **GDI+ ile PNG/JPG/GIF/TIFF/ICO** yükleyebiliyor.

```cpp
// PNG / JPG / GIF / BMP / TIFF / ICO — hepsi:
pictureBox1->Image = M2God::Image::FromFile(L"C:\\\\resim.png");

// Tek satır kolay yol:
pictureBox1->Load(L"resim.jpg");            // bool döner: true = yüklendi

// Tasarımcıda ImageLocation özelliğine yol yazın — yaratımda otomatik yüklenir
pictureBox1->ImageLocation = L"resim.png";

// Boyutlandırma (5 mod):
pictureBox1->SizeMode = M2God::PictureBoxSizeMode::Zoom;  // Normal | StretchImage | AutoSize | CenterImage | Zoom

// Eski HBITMAP yolu da çalışmaya devam eder:
pictureBox1->Image = (HBITMAP)LoadImageW(0, L"r.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
```

**Teknik:** `gdiplus.dll` **dinamik yüklenir** (LoadLibrary + flat API) — ne ek `.lib` ne `gdiplus.h` ne proje ayarı gerekir; VS2019 ve mingw'de olduğu gibi derlenir. StretchImage/Zoom modlarında HighQualityBicubic ölçekleme; WM_NCDESTROY'da resimler otomatik serbest bırakılır.

**Doğrulama:** Üretilen proje C++14 **ve** C++17 ile 0 hata; wine altında PictureBox'a JPG (`ImageLocation`) **ve** PNG (`M2God::Image::FromFile`) yüklendi — her ikisi de ekranda görüntülendi (EkranGoruntuleri_Faz2.5.2/).

---

## Faz 2.5.1 — VS2019 VARSAYILAN (C++14) uyumluluk düzeltmesi

Kullanıcı geri bildirimi: v2500 Global.h, VS2019'un **varsayılan** proje ayarı olan **/std:c++14**'te derlenmiyordu:

| Hata | Sebep | Çözüm (v2501) |
|------|-------|----------------|
| **C2864 / C7525 / E2860** (`M2God::Form::*`) | `inline static` üyeler C++17 gerektirir | `Form`, **sınıf şablonu tekniğine** taşındı: `template<typename D> struct FormBase` + `using Form = FormBase<void>` — statik üye tanımları şablon oldukları için header-only kalır, **sözdizimi değişmez** (`M2God::Form::Handle`, `M2God::Form::Text = ...`) |
| **C4996** (`wcsncpy`, 5 satır) | Güvensiz CRT uyarısı (hata olarak) | `M2God::M2StrNCpy` yardımcısı: aynı davranış, sıfır uyarı, ek tanım gerekmez |
| **E0245** (IntelliSense, diyalog satırları) | inline static'in ardındaki zincir hata | FormBase tekniğiyle kendiliğinden temizlendi |

**Doğrulama:** Üretilen proje (17 kontrol + 4 bileşen) `x86_64 g++ -std=c++14` ile **0 hata 0 uyarı** derlendi; wine altında çalıştırıldı: `timer1->Enabled=false` ile sayaç **Tick 98'de kalıcı durdu**, buton "Durduruldu" + gri oldu. `/std:c++17` ile de aynı sonuç. **Proje ayarı değiştirmeye gerek yok** — tasarımcı bir sonraki Senkron'da (F7) Global.h'ı otomatik v2501'e yükseltir (olay gövdeleri korunur).

---

## Faz 2.5 — TAM KAPSAMLI M2God SDK (Global.h v2501)

Config_info.json'daki **tüm** toolbox elemanları artık üretilen projede nesne sözdizimiyle kullanılabiliyor. Global.h ~3200 satırlık header-only SDK:

### Kontroller (tasarım özellikleri de koda taşınıyor)
- **TextBox/MaskedTextBox**: ReadOnly, PasswordChar, MaxLength, TextAlign, CharacterCasing, UseSystemPasswordChar, WordWrap + AppendText/Clear/Copy/Cut/Paste/Undo/SelectAll
- **NumericUpDown**: gerçek EDIT+msctls_updown32 — Value/Minimum/Maximum/Increment/ReadOnly/UpButton/DownButton
- **RichTextBox**: RTF SaveFile/LoadFile (EM_STREAM), SelectionColor, SelectedText, Undo/Redo, BackColor (EM_SETBKCOLOR)
- **ComboBox**: Items->Add/AddRange/Insert/RemoveAt/IndexOf/Contains, SelectedIndex, SelectedItem, DropDownStyle, DroppedDown, MaxLength, BeginUpdate/EndUpdate
- **ListBox**: Items API + SelectionMode, TopIndex, ItemHeight, GetSelected/SetSelected/ClearSelected
- **CheckedListBox**: SysListView32 (LVS_LIST+checkboxes) — Items->Add, SetItemChecked/GetItemChecked, CheckedItemCount
- **ListView**: Columns->Add(title,width), Items->Add(text,sub), SetSub, View/CheckBoxes/GridLines/FullRowSelect/HeaderStyle, EnsureVisible, item checkbox
- **TreeView**: Nodes->Add/AddChild/RemoveAt/ExpandAll, SelectedIndex/SelectedText, CheckBoxes + SetItemChecked, ShowLines/HotTracking/Indent
- **ProgressBar**: Value/Minimum/Maximum/Step, PerformStep/Increment, Style (Marquee)
- **TrackBar**: Value/Minimum/Maximum/TickFrequency/SmallChange/LargeChange/TickStyle/SetRange
- **HScrollBar/VScrollBar**: Value/Minimum/Maximum/SmallChange/LargeChange (SetScrollInfo)
- **DateTimePicker**: Value (M2DateTime), Format (Long/Short/Time/Custom), CustomFormat, Checked
- **MonthCalendar**: SetDate, MaxSelectionCount, TitleBackColor/TitleForeColor/TrailingForeColor, FirstDayOfWeek
- **TabControl**: Pages/TabPages->Add/Insert/RemoveAt, SelectedIndex, HotTrack (Text satırları sayfa olur)
- **StatusStrip**: Parts/Items->Add/SetText/Clear (SB_SETPARTS ile düzgün parçalar)
- **MenuStrip/ToolStrip**: gerçek HMENU — Items->Add/AddChild + Apply() + ItemClick (WndProc DispatchCommand)
- **ContextMenuStrip**: Items API + Show(x,y) (TrackPopupMenu) + ItemClick
- **Panel**: BorderStyle (FixedSingle/Fixed3D/None yeniden stillenir)
- **PictureBox**: özel M2GOD_PIC sınıfı — **Image (M2God::Image — GDI+ PNG/JPG/GIF; ya da HBITMAP)**, Load(path), ImageLocation, SizeMode (Normal/StretchImage/AutoSize/CenterImage/Zoom)
- **Label/LinkLabel/Button/CheckBox/RadioButton/GroupBox**: TextAlign (ContentAlignment), BorderStyle, AutoEllipsis, FlatStyle, CheckState (Indeterminate), PerformClick

### Bileşenler ve diyaloglar (görsel olmayan ama GERÇEK çalışan)
- **OpenFileDialog/SaveFileDialog**: Filter/FileName/Title/InitialDirectory/Multiselect, ShowDialog() → M2God::DialogResult::OK
- **ColorDialog**: Color/FullOpen (ChooseColorW), **FontDialog**: Font/FontName/FontSize (ChooseFontW)
- **FolderBrowserDialog**: SelectedPath/Description (SHBrowseForFolderW)
- **NotifyIcon**: Icon/Text/Visible/BalloonTip + ShowBalloonTip, Click/DoubleClick (WM_APP+0x4E1 yönlendirmesi)
- **ToolTip**: SetToolTip(control, text), Active, InitialDelay/AutoPopDelay/ReshowDelay, ToolTipTitle
- **BackgroundWorker**: DoWork/ProgressChanged/RunWorkerCompleted (std::function), RunWorkerAsync/ReportProgress/CancelAsync — gerçek thread
- **Process**: FileName/Arguments/Start/WaitForExit/ExitCode/Kill + statik Process::Start(L"notepad.exe")
- **SerialPort**: PortName/BaudRate/Open/Close/Write/WriteLine/ReadExisting (CreateFileW COM)

### Form API, enumlar, renkler
- **M2God::Form**: Handle/Text/ClientSize/BackColor/TopMost/Opacity(0-100)/WindowState/MaximizeBox/MinimizeBox/ControlBox/FormBorderStyle/Close/Activate/CenterToScreen
- **M2God::Color**: 30+ sabit (Red, Lime, Navy, LightGray, Transparent...) + FromArgb
- **enum class'lar** (WinForms değerleri): ComboBoxStyle, View, SortOrder, PictureBoxSizeMode, ProgressBarStyle, DateTimePickerFormat, SelectionMode, CharacterCasing, ScrollBars, HeaderStyle, ContentAlignment, MessageBoxButtons/Icon, DialogResult, FormBorderStyle/StartPosition/WindowState, Keys, RichTextBoxStreamType...
- **MsgBox(text, caption, buttons, icon)** → DialogResult
- **Point/Size/Rectangle/M2DateTime** yapıları; Location/Size/Width/Height/Left/Top/Right/Bottom property'leri; Focus/Refresh/BringToFront/SendToBack/SetBounds

### Olay yönlendirme genişletildi
- WM_COMMAND: Click/DoubleClick/TextChanged/SelectedIndexChanged/ValueChanged (STN_CLICKED, LBN_DBLCLK, CBN_DBLCLK, CBN_EDITCHANGE dahil)
- **WM_NOTIFY** (yeni): ListView/CheckedListBox → LVN_ITEMCHANGED (SelectedIndexChanged + ItemCheck), TreeView → TVN_SELCHANGED (AfterSelect), TabControl → TCN_SELCHANGE, DateTimePicker → DTN_DATETIMECHANGE
- **WM_HSCROLL/WM_VSCROLL** (yeni): TrackBar/ScrollBar → Scroll + ValueChanged
- **Form olayları** (yeni): Shown, Resize, FormClosing, FormClosed, Activated, MouseMove/MouseDown/MouseUp/Click, KeyDown/KeyUp/KeyPress
- Yaratım sırası bildirim koruması: `M2God::InitDone()` — InitializeControls bitmeden hiçbir handler çağrılmaz (zincirleme EN_CHANGE çökmesi kökünden giderildi)
- Form tasarım özellikleri wWinMain'e taşınır: StartPosition=CenterScreen, MaximizeBox/MinimizeBox/ControlBox, FormBorderStyle, TopMost, WindowState

## Faz 2.3 — Yenilikler (kullanıcı geri bildirimi üzerine)

| Düzeltme / Yeni Özellik | Ayrıntı |
|---|---|
| **PID → .vcxproj LİSTESİ (dosya konumu seçme yok)** | PID seçilince çözüm/ proje konumu **otomatik** bulunur: pencere başlığı → çözüm adı → diskte .sln taraması (gevşek eşleşmeli) → .sln ayrıştırması → bulunamazsa klasörde .vcxproj taraması. Bulunan projeler **görünür bir liste diyaloğunda** sunulur (çift tık / butonla seç). Hiç bulunamazsa son çare dosya seçme diyaloğu |
| **Tasarım kalıcılığı (proje klasöründe .m2god)** | Tasarım, bağlı projenin klasörüne **otomatik kaydedilir** (Model değişince 1.2 sn debounce + çıkışta kaydet). Aynı PID'e yeniden bağlanınca **kayıtlı tasarım algılanır** ve seçenek sunulur: **Önceki Tasarımı YÜKLE / YENİ FORM — öncekini KORU / YENİ FORM — öncekini SİL / İptal**. "VS'e bağlan - yeni form aç" yerine artık tek **"VS'ye Bağlan"** adımı var — seçenekler bağlandıktan sonra gelir |
| **Overlay sürükleme — hayalet piksel bug'ları bitti** | Taşıma sırasında gerçek HWND **artık oynatılmaz**; kontrol gizlenir, yüzeyin üstündeki **bölgeli katman penceresi** (SetWindowRgn) hayaleti çizer. Grup kutusunun içinde "kendini kopyalamış" görüntü, "binlerce buton" kalıntısı gibi tüm artefaktlar kökünden giderildi. WS_CLIPCHILDREN kaldırıldı; boyutlandırma/klavye taşıma/kaydırma tam yeniden boya ile temiz |
| **Konteynerden akıcı çıkış** | Taşıma sırasında konteyner clamp'i KALDIRILDI: kontrol konteynerin dışına tek hamlede serbestçe çıkar (eski davranışta kenara çarpıyor, sürükleme kesiliyor, tekrar tutmak gerekiyordu). Hedef katman bırakma anında imleçten belirlenir; **bırakma hedefi kesikli mavi çerçeveyle canlı gösterilir** |
| **Bırakma hedefi vurgusu** | Sürükleme sırasında imlecin altındaki konteyner mavi kesikli çerçeveyle işaretlenir — kontrolün nereye alınacağı bırakmadan ÖNCE görünür. TabControl'ün sekme şeridine bırakılanlar içeri alınmaz (sayfa alanı kuralı) — yanlışlıkla yutma sürprizi bitti |
| **Araç kutusu tıklama görseli DÜZELTİLDİ** | Araca tıklayınca vurgu artık aracın kendisinde; **kategori satırı asla seçilmiyor** (TVN_SELCHANGED düzeltmesi). Armed araç, yerleştirilene/ESC'ye kadar vurgulu kalır (VS davranışı) |
| **Silinen nesneler ekrandan kalkıyor** | Kök neden: `Model::Delete` gerçek HWND'yi yok etmiyordu — hayalet ekranda kalıyordu. Artık silme alt kontrollerle birlikte HWND'leri de yok eder; "Yeni Tasarım" HWND sızıntısı da giderildi |
| **Renk/Yazı tipi/Boyut/enum düzenleme çalışıyor** | Kök neden: `CBN_SELCHANGE / CBN_KILLFOCUS / EN_KILLFOCUS` bildirimleri **WM_COMMAND ile gelir** — eski kod bunları WM_NOTIFY içinde arıyordu, hiç tetiklenmiyordu. Ayrıca: renk kombosu açılır liste + **Özel renk... (ChooseColor)**, yazı tipi kombosu + **Yazı tipi seç... (ChooseFont)**, enum/bool seçimi anında işler, liste kaydırılınca açık düzenleyici güvenle kapanır (LVN_BEGINSCROLL) |
| **Z-sırası garanti** | Oluşturma sırası = z-sırası (son eklenen üstte) — yeniden bağlanma/yükleme sonrası da korunur; aynı-katman bırakmalarda HWND model konumuna zorla hizalanır |
| **Geniş dize düzeltmesi** | mingw derlemede `swprintf %s` C99 anlamıyla multibyte okuyordu (PID satırları tek harf görünüyordu) → tüm biçimler `%ls` |

### Faz 2.3 doğrulama kanıtları (Wine + otomasyon, sahte devenv ile uçtan uca)
- Sahte devenv.exe (başlık: "MayFarm - Microsoft Visual Studio") + diskte `source/repos/MayFarm/MayFarm.sln` kurgusu
- Karşılama → PID seç → **Proje Seç listesi** (2 vcxproj tam yollarıyla) → Bağlan → `Form1.h/.cpp/Form1.m2god` proje klasöründe oluşturuldu
- Buton yerleştirildi → **"Otomatik kaydedildi: Form1.m2god"** → uygulama kapatılıp açıldı → aynı PID'e bağlanınca **"Bu Proje İçin Kayıtlı Tasarım Var"** → YÜKLE → buton geri geldi
- Araç tıklama vurgusu araçta; hayalet sürükleme katmanı konteyner üzerinde temiz; tek hamlede konteynerden çıkış (Location 224,-112 → clamp yok); bırakınca içine/dışına alma doğru
- Silme → kontrol tamamen kayboldu; ClientSize 700,500 → form küçüldü; BackColor "Red" → tuval kırmızı; MaximizeBox → Down ile False anında işlendi
- Z-sırası: önce GroupBox, sonra TabControl → **TabControl üstte**, grup çerçevesi altta kırpılıyor

## Faz 2.2 — Yenilikler (kullanıcı geri bildirimi üzerine)

| Düzeltme | Ayrıntı |
|---|---|
| **"Özellikler/Olaylar'da her şey seçili" DÜZELTİLDİ** | Kök neden: `ListView_SetItemText` makrosu **maskesiz (uninitialized) LV_ITEM** gönderiyordu; çöp mask `LVIF_STATE` bitini içerince satırlara rastgele `LVIS_SELECTED` yapışiyordu. Artık açık maskeli güvenli setter kullanılıyor + seçim durumu tasarımcı tarafında takip ediliyor (`CDIS_SELECTED`'a güvenilmez) |
| **Özellik değişimi artık çalışıyor** | Satır içi düzenleyici ENTER ile işler, **ESC** vazgeçer; hücreye tıklayınca açılan kutu değeri **seçili** getirir (yazınca eskisi değişir — VS davranışı). Değişim anında tuval ve kod güncellenir |
| **Düzenleme kutusu "aşırı yukarda" DÜZELTİLDİ** | Kutu, liste-client koordinatları panele `MapWindowPoints` ile çevrilerek **tam satırın üstünde** açılır; aynı satıra tekrar tıklamak düzenlemeyi bozmaz |
| **C2040 derleme hatası DÜZELTİLDİ** | `SyncIntoFile` HANDLES bölgesini **tipsiz** (`button1 = NULL;`) yeniden yazıyordu → global alanda geçersiz. Artık `HWND button1 = NULL;` üretir (ilk üretim de senkron da tutarlı) |
| **Butona tıklayınca MessageBox çıkmıyordu — DÜZELTİLDİ** | Senkron WNDPROC bölgesini tazelemiyordu; yeni bağlanan olaylar **boş `switch(id)`** içinde kalıyordu. Artık WM_COMMAND yönlendirme tablosu her senkronda güncellenir: `case M2GodForm::IDC_BUTTON1: if (ev == BN_CLICKED) M2GodForm::button1_Click(hWnd);` |
| **Konteyner bildirim köprüsü** | GroupBox/TabControl/Panel içine yaratılan çocukların `WM_COMMAND/WM_NOTIFY`'ı köprü prosedürle ana forma taşınır (`SetWindowSubclass`) — iç içe kontrollerin olayları da çalışır |
| **Çift `// Timer:` üretimi giderildi** | Bileşenler CREATE bölgesinde tek kez üretilir |
| **Kolon taşması giderildi** | Özellik adı kolonu başlık genişliğine kırpılır (GetSubItemRect(0) satırın tamamını döndürdüğü için uzun adlar Değer kolonuna taşıyordu) |
| **Olay çift tıkı korundu** | Aynı satır düzenleme koruması, olay satırındaki çift tıkla handler üretimini engellemiyor |

### Faz 2.2 doğrulama kanıtları (Wine + otomasyon)
- Boş form → tıkla-yerleştir → özellik paneli düzgün (hersatı beyaz, tek satır seçili)
- Text hücresi → kutu **tam satırda** + değer seçili → `Deneme` yazılır → ENTER → tuvalde buton metni değişti
- Olaylar sekmesi → Click çift tık → `button1_Click` otomatik üretildi
- F6 üretim → üretilen `Form1.cpp`: `HWND button1 = NULL;` + WM_COMMAND case + konteyner köprüsü + iç içe yaratım
- Üretilen kod mingw ile derlendi → Form1.exe çalıştı → **butona tıklayınca MessageBox (Mause/Test) ortaya çıktı**
- `SyncIntoFile` birim testi (tests/harness.cpp): kullanıcının C2040'lı bozuk Form1.cpp'si senkronla onarıldı; **kullanıcı kodu (MessageBoxA gövdesi) korundu**; 8/8 doğrulama geçti; onarılan dosya derlendi

---

## Faz 2.1 — Yenilikler

| Düzeltme / Yeni Özellik | Ayrıntı |
|---|---|
| **Karşılama + VS Seçim Ekranı** | Program açılışta önce **çalışan VS örneklerini** listeler (PID + çözüm + proje adı). Seçilen örneğe bağlanır, **Form1.h/Form1.cpp'i o VS projesinin klasöründe oluşturur** ve **SIFIR (boş) form** açar. "Önceki tasarımı aç" ile eski .m2god/.cpp yüklenir, "VS'siz devam et" ile boş form açılır |
| **Yönetici modu uyumlu VS taraması** | Tarama artık `devenv.exe` süreçlerini **Toolhelp32** ile bulur + ana pencere başlığından çözüm adını okur (yüksek bütünlük seviyesinde de çalışır). DTE (ROT) varsa zenginleştirir; yoksa pencere modu |
| **PID seçince çökme DÜZELTİLDİ** | VS bağlan diyaloğu artık gerçek **modal** + veri heap'te (use-after-free giderildi); çok projetli çözümde proje seçim menüsü |
| **Konteyner iç içe (parent-child)** | Panel / GroupBox / TabControl / FlowLayoutPanel içine **kontrol bırakılabilir**. Çocuk konteynerin gerçek HWND çocuğu olur: **konteyneri sürükleyince çocuklar birlikte gelir**, koordinatlar parent-görelisi tutulur ve üretilen koda da aynı hiyerarşi yansır |
| **Sürükleyip içine/dışına alma** | Bir kontrolü sürükleyip GroupBox/Panel üstüne bırakınca **içine alınır**; dışarı çıkarınca forma döner (VS davranışı) |
| **Z-sırası kuralı** | Oluşturma sırası = z-sırası (son eklenen üstte). Hit-test parent zincirine duyarlı: üstteki kontrol seçilir, konteyner değil çocuğu taşınır |
| **Tıkla-Yerleştir (VS gibi)** | Araç kutusunda nesneye **tek tık** → imleç formda artı (cross) olur → tıklanan yere nesne eklenir → imleç normale döner. **ESC** her odakta iptal eder |
| **Araç kutusu arama** | Üstteki arama kutusuna yazınca liste anlık filtrelenir |
| **Sağ tık bağlam menüsü** | Sil / Kes / Kopyala / Yapıştır / Öne Getir / Arkaya Gönder + varsayılan olay işleyicisi — Silme artık her koşulda çalışır |
| **Silme tuşu düzeltmeleri** | Delete hem yüzeyde hem hızlandırıcıyla güvenli; metin düzenleyicideyken (özellik paneli) yanlışlıkla kontrol silinmez; Ctrl+C/X/V artık edit kutularında metin kopyalar |
| **Flicker giderildi** | Sürükleme sırasında özellik paneli yeniden kurulmaz — yalnızca Location/Size değerleri canlı güncellenir |
| **Sıfır form** | "Yeni" gerçekten boştur; açılışta demo nesneler yoktur (eski demo `--demo` bayrağıyla açılır) |
| **CodeGen parent-aware** | Üretilen `CreateWindowExW` çağrıları konteyner çocuğunu konteynerin HWND'sine yaratır; BS_GROUPBOX/BS_AUTOCHECKBOX/BS_AUTORADIOBUTTON stil metinleri düzeltildi |
| **Diyalog arka planı** | Bağlan/kod diyaloğundaki siyah arka plan düzeltildi |

---

## Neler Var?

| Özellik | Açıklama |
|---|---|
| **Araç Kutusu** | Config_info.json'daki tüm kategoriler (Ortak Denetimler, Kapsayıcılar, Menüler ve Araç Çubukları, Veri, Bileşenler, İletişim Kutuları, Yazdırma...) — Türkçe kategori adlarıyla |
| **Sürükle-Bırak** | Araç kutusundan tasarım yüzeyine tut-sürükle-bırak; çift tıkla-ekle de çalışır |
| **Gerçek Kontroller** | Yüzeyde gerçek Win32 HWND'leri oluşur: Button, TextBox, CheckBox, RadioButton, ComboBox, ListBox, ListView, TreeView, GroupBox, Panel, ProgressBar, TrackBar, DateTimePicker, MonthCalendar, TabControl, RichTextBox, PictureBox, StatusBar, MenuStrip (placeholder) + Timer gibi bileşenler için **Bileşen Tepsisi** |
| **Seç / Taşı / Boyutlandır** | Tek tık seçim, 8 tutamaçlı boyutlandırma (VS gibi), fareyle taşıma, ok tuşlarıyla ince ayar (Shift+ok = boyut), 8px ızgara yapışma |
| **Özellikler Paneli** | Config_info.json'dan gelen **tüm browsable+can_set property'ler** (Name, Text, Location, Size, Font, ForeColor, BackColor, Anchor, FlatStyle, TextAlign...), Boolean/enum için açılır liste, Color için hazır palet |
| **Olaylar Paneli** | Seçili tipin **tüm browsable event'leri** (Click, TextChanged, CheckedChanged, MouseMove...); çift tıkla → varsayılan işleyici adı üretilir ve bağlanır |
| **Kod Üretimi (F6)** | Marker'lı **Form1.h + Form1.cpp** üretir — VS CLR projesi düzeninde ama saf Win32 |
| **Kod Önizleme (F12)** | Üretilen kodu anında gör, panoya kopyala |
| **Kaydet/Aç (.m2god)** | Tasarım JSON olarak saklanır, geri yüklenir |
| **Faz1.4 İçe Aktarma** | Eski `GeneratedForm.cpp` (CreateWindow satırları) dosyasını seç → tasarıma dönüştürür |
| **Undo/Redo** | Ctrl+Z / Ctrl+Y (100 adım) |
| **VS2019 Entegrasyonu** | Aşağıda ayrıntılı |

---

## Visual Studio Entegrasyonu (God Mode)

**Proje → Visual Studio'ya Bağlan... (Ctrl+Shift+V)**

1. Çalışan tüm `devenv.exe` örnekleri listelenir:
   - **DTE (COM) yolu:** Running Object Table üzerinden `VisualStudio.DTE.16.x` — PID, sürüm, **çözüm adı**, pencere tutamacı
   - **Pencere (fallback) yolu:** `devenv` sınıfı pencerelerinden başlık/PID eşleştirme
2. Örnek seçilir → çözümdeki `.vcxproj`'lar ayrıştırılır, proje seçilir (yoksa dosya diyaloğu ile klasör seçilir)
3. Bağlanınca projede **Form1.h / Form1.cpp** yoksa üretilir, varsa **marker blokları üzerinden artımlı senkron** yapılır:
   - `//== M2GOD:IDS-BEGIN/END` — kontrol ID enum'u
   - `//== M2GOD:HANDLES-BEGIN/END` — HWND tanımları
   - `//== M2GOD:CREATE-BEGIN/END` — InitializeControls gövdesi
   - `//== M2GOD:EVENTDECL-BEGIN/END` — işleyici bildirimleri
   - `//== M2GOD:EVENTS-BEGIN/END` — işleyici gövdeleri (**mevcut kodunuz asla silinmez**, yalnızca eksik stub eklenir)
4. **Nesneye çift tıkla** → varsayılan event (Button→Click, TextBox→TextChanged, **Form→Load**, Timer→Tick...) işlenir:
   - Handler Form1.cpp'ye eklenir (varsa olduğu yerde bırakılır)
   - **Visual Studio öne getirilir** (AttachThreadInput hilesi + SetForegroundWindow)
   - DTE varsa `ItemOperations.OpenFile` + `ExecuteCommand("Edit.GoTo", satır)` ile **handler'ın satırına gidilir**; DTE yoksa Ctrl+G tuşlamasıyla aynı sonuç
5. `.vcxproj` dosyasına `Form1.h/Form1.cpp` girdileri eklenir (VS harici değişikliği algılayıp yeniden yükler)

> VS'nin kendisi gerekmez: kod üretimi (F6) bağımsız çalışır ve çıktı herhangi bir Win32 projesine eklenip derlenebilir.

---

## Derleme

### Visual Studio 2019 (önerilen)
1. `M2GodWin32.sln`'yi aç (v142 toolset, Unicode, C++17, `/utf-8`)
2. F7 → Bin\M2GodWin32.exe
3. `Config_info.json`'ı exe'nin yanına koy (katalog otomatik yüklenir; yoksa yerleşik mini katalog devreye girer)

### mingw-w64 (MSYS2 / Linux cross)
```bash
cd M2GodWin32
bash build_mingw.sh        # → Bin/M2GodWin32.exe (statik, bağımlılıksız)
```

---

## Kullanım Akışı

```
Araç kutusundan sürükle → yüzeye bırak → seç → taşı/boyutlandır
  → Özellikler sekmesinden değerleri düzenle (Enter / odağı kaydır)
  → Olaylar sekmesinde olaya çift tıkla (handler oluşur)
  → Ctrl+Shift+V ile VS'ye bağlan (VS açıksa)
  → Çift tıkla → VS'de handler'ın içinde ol!
  → F6: Form1.h/Form1.cpp'yi klasöre üret → VS'de derle
```

**Kısayollar:** Ctrl+N/Yeni · Ctrl+O/Aç · Ctrl+S/Kaydet · F6/Kod Üret · F12/Önizleme · F7/VS Senkron · Ctrl+Shift+V/VS Bağlan · Ctrl+Z/Y Geri-Yinele · Ctrl+X/C/V Kes-Kopyala-Yapıştır · Del/Sil · Ctrl+T/B Öne-Arkaya · Ok/Shift+Ok Taşı-Boyutlandır · Esc/Seçimi bırak

---

## Üretilen Kod Örneği

```cpp
//== M2GOD:CREATE-BEGIN
void InitializeControls(HWND hWnd) {
    HINSTANCE hInst = GetModuleHandleW(NULL);
    EnsureStatics(hInst);
    textBox1 = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"Merhaba M2God",
        WS_BORDER | ES_AUTOHSCROLL | WS_CHILD | WS_VISIBLE | WS_TABSTOP, 24, 60, 220, 26,
        hWnd, (HMENU)(INT_PTR)IDC_TEXTBOX1, hInst, NULL);
    SendMessageW(textBox1, WM_SETFONT, (WPARAM)hFont, TRUE);
}
//== M2GOD:CREATE-END
```

`OrnekCikti/` klasöründe tasarımın ürettiği gerçek Form1.h/Form1.cpp vardır — **derlenip çalıştığı kanıtlanmıştır** (EkranGoruntusu_7).

---

## Sınırlar (MVP)

- Kapsayıcı iç içe geçme (Panel içine kontrol) görseldir; hiyerarşik parent-child üretimi sonraki faza bırakıldı
- Image/ImageList görselleri metin olarak saklanır
- DataGridView/WebBrowser gibi COM tabanlı kontroller placeholder çizilir (event/property düzenlenir)
- MenuStrip/ToolStrip placeholder'dır; gerçek menü üretimi sonraki faz

## Dosya Düzeni

```
M2GodWin32.sln
M2GodWin32/
  M2GodWin32.vcxproj(.filters)   ← VS2019 projesi
  build_mingw.sh                 ← mingw derleme betiği
  build/  app.rc · app.manifest · app.ico
  src/    main.cpp · app.h · utils.cpp · json.cpp/h · catalog.cpp/h ·
          model.cpp/h · factory.cpp/h · surface.cpp/h · toolbox.cpp/h ·
          props.cpp/h · codegen.cpp/h · vsint.cpp/h · mainwnd.cpp/h
  Bin/    M2GodWin32.exe · Config_info.json
OrnekCikti/                      ← üretim kanıtı (derlenebilir)
EkranGoruntuleri/                ← 8 adet kanıt görüntüsü
```
