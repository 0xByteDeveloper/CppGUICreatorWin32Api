# CppGUICreatorWin32Api

**Win32 API ile saf C++ GUI tasarımcısı** — Visual Studio 2019 Windows Forms deneyiminin .NET'siz, native yeniden inşası.

## 📚 Belgeler

| Doküman | İçerik |
|---|---|
| [KULLANICI_ISTEGI.md](KULLANICI_ISTEGI.md) | Projenin kurucu vizyonu — birebir kullanıcı isteği |
| [PLAN.md](PLAN.md) | Mimari plan: injection↔standalone kararı, tasarımcı UI spec, kod üretimi, VS 2019 entegrasyonu (EnvDTE), faz planı |

## Config_info.json

**Visual Studio 2019 C++/CLR (Windows Forms) tüm eleman kataloğu** — Win32 API GUI üretici için yapılandırma kaynağı.

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
    { "name": "Text", "type": "System.String", "category": "Appearance",
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
