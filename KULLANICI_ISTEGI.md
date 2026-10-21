# 📋 Kullanıcı İsteği — Kavram Kaydı (Verbatim)

| | |
|---|---|
| **Kayıt tarihi** | 2026-10-21 |
| **İsteği yazan** | 0xByteDeveloper |
| **İlgili doküman** | [PLAN.md](PLAN.md) — bu isteğin mimari karşılığı |
| **Amaç** | Bu yazı, projenin kurucu vizyonudur; aynen korunmuştur. |

---

## Orijinal İstek (birebir)

> C++ CLR Form .NetFramework kullanıyordu menüyü göstermek için ,
> biz kaynak kodumuzun okunmaması için Win32Api ile yapıcaz aynısını,
>
> Yine Bir Form görebilicez, Nesne ekleme penceresini görücez (galiba solda) aynı visual studiodaki gibi,
>
> Ve obje ekleyebilicez, sürükeyebilicez locationunu değiştirebilicez , boyutunu büyütüp küçültebilicez,
> birbirinin içien koyabilicez panel in içi gibi ,
>
> Ve nesneyi seçtiğimizde ayarlarını da değiştirebilicezğiz aynı visual studio 2019 daki gibi sağda Properties simgesi var ve Events simgesi var ,
> Propertieste Name, String Location , .... gibi şeyler var ,
> Events te Paint ,Onclick gibi şeyler var
> Bunlar yoksa bunlarının bilgisini çek Visual studio dan.
> (derin araştırabilirsin visual studiodan bilgi çekebilirsin veriseti şeklinde işe yarar)
> Visual studio 2019 C++ CLR Form da solda nesne ekleyen araç tool (searchtextbox var eklceim nesneyi bulurken kolaylık sağlıyor )
> ,sağda 2 yarım pencere var yarısı yani üstte dosya gezgini, altta Özellikler yani Seçili nesnenin özellikleri ve içinde simgeler var , properties ve event ve kolaylık için a dan z ye sırala gibi bi simge var veya kategoriye göre sırala seçeneği var.
>
>
> Biz birebir aynı şekilde menü oluşturabilicez ve vs2019 C++ ile derleyebilicez, fakat vs 2019C++ de bizim menümüzü görebilelim yani dll mi injectleyelim menümüzü görmek için yoksa farklı bi UI oluşturucu pencere mi yapalım ? farklı pencerede yaparsam proje vs2019 ile bağlı olur ben nesne eklediğimde kendine özel cpp yi düzenler o cpp vs 2019 da da açıktır ve ben o UI oluşturucu penceremizden Nesneye Paint olarak tıklarsam beni bağlı olduğumuz vs2019 TopMost olarak öne gelir, ve Hedef.cpp focused olur, kapatılmışsa açılır,
> açıldığı doğrulandıktan sonra o form a ait hedef .cpp de daha önce pictureBox_Paint sınıfı yoksa kod ekleyeceksin varsa oraya scroll edeceksin.
>
> Ne yapalım önce bunu planla .md dosyasına Plan oluştur.
> bu yazdım çok güçlü bi kavram o yüzden sana yazdığım bu yazıyı da githubda bi md ye kullanıcıistegi olarak kaydet.

---

## İstekten Çıkan Somut Gereksinimler (referans)

1. Tasarımcı **saf Win32 API + C++** (kaynak okunamaz, .NET bağımlılığı yok)
2. VS2019 birebir düzen: **sol** Toolbox (+arama), **orta** tasarım yüzeyi, **sağ üst** dosya gezgini, **sağ alt** Özellikler (Properties/Events simgeleri, A-Z ve kategori sıralama)
3. Obje ekleme / sürükleme / konum & boyut / **iç içe yerleştirme** (Panel içi gibi)
4. Property'ler: Name, Text, Location, ...
5. Event'ler: Paint, OnClick, ...
6. Bu bilgiler **Visual Studio verisinden** çekilir (→ `Config_info.json` veri seti bu amaçla üretildi)
7. Mimarî karar: **DLL injection yerine bağımsız UI oluşturucu pencere** (PLAN.md Bölüm 2'de gerekçeleriyle)
8. Nesne eklendiğinde bağlı projenin **Hedef.cpp**'si otomatik düzenlenir
9. Event'e tıklanınca: VS2019 **TopMost öne gelir** → `Hedef.cpp` **focus** (kapalıysa açılır) → stub yoksa **eklenir**, varsa **o satıra scroll edilir**
