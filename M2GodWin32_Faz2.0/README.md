# M2GodWin32 — Faz 2.0 God Mode MVP

**Visual Studio 2019 C++ CLR Form Designer deneyimini Win32 API ile yaşatan, tam kapsamlı görsel UI tasarım ortamı.**

> Bu proje `CppGUICreatorWin32Api` deposunun **Faz 2.0** geliştirme aşamasıdır.
> Katalog kaynağı olarak depodaki **Config_info.json** (.NET Framework WinForms tam eleman kataloğu: 144 eleman, tüm property'ler + event'ler + 203 enum) kullanılır.

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
