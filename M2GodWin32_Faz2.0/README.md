# M2GodWin32 — Faz 2.2 God Mode MVP

**Visual Studio 2019 C++ CLR Form Designer deneyimini Win32 API ile yaşatan, tam kapsamlı görsel UI tasarım ortamı.**

> Bu proje `CppGUICreatorWin32Api` deposunun **Faz 2.2** geliştirme aşamasıdır.
> Katalog kaynağı olarak depodaki **Config_info.json** (.NET Framework WinForms tam eleman kataloğu: 144 eleman, tüm property'ler + event'ler + 203 enum) kullanılır.


---

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
