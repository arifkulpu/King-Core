# King Core

King Core, Skyrim SE/AE için SKSE tabanlı bir eklenti ve uyum katmanıdır. Birbirinden bağımsız modların aynı takipçi sınıfı, davranış profili ve envanter kurallarını kullanmasına yardımcı olur. Kendisi yeni sınıf içerikleri veya takipçi AI'sı sunmaz; bu işlevleri sağlayan ve kullanan modlar arasında veri taşır.

## Ne işe yarar?

- Sınıf sağlayıcısından takipçinin sınıfını ve davranış profilini alır.
- Bu verileri Follower Unbound gibi tüketici modlara iletir.
- Modların King Core'a bağlanma durumunu takip eder ve durum panelinde gösterir.
- Tedavi öğelerini kayıt altına alıp tüketici modlara bildirir; böylece bu öğelerin genel envanter/satış işlemlerinde korunabilmesini sağlar.
- Soul gem yönetiminin başka bir moda ait olduğunu bildirerek tüketici modların kendi soul gem ekonomisini devre dışı bırakmasına olanak verir.
- Follower Unbound'un uyku/bekleme sonrası takipçilerin dinlendiği bilgisini Life Again gibi tüketici modlara iletir.

## Uyumlu modlar ve bağlantı amaçları

### ClassBound → King Core → Follower Unbound

ClassBound, takipçi sınıfı bilgisini sağlayan taraftır. King Core, ClassBound API'sini alır ve takipçi için sınıf/ davranış profili sorgularını Follower Unbound'a sunar. Follower Unbound bu profili sınıfına uygun ekipman ve eşya öncelikleri belirlemek için kullanabilir. Sınıf sağlayıcısı yoksa King Core temel bir beceri değerlendirmesinden geçici Warrior, Archer, Mage veya Assassin profili üretir.

King Core ClassBound API'sinden sınıf atama çağrısını da dışa açar; bu özelliği kullanmak tüketici modun uygulamasına bağlıdır.

### Follower Unbound

Follower Unbound, King Core'dan sınıf davranış profillerini ve sınıf değişikliği bildirimlerini tüketir. Ayrıca King Core'a API isteği göndererek varlığını bildirir. King Core'un panelindeki bağlantı işareti bu el sıkışma mesajı alındığında etkinleşir.

### Life Again

Life Again, kullandığı tedavi/yaralanma öğelerinin FormID'lerini King Core'a kaydedebilir. King Core bu kaydı Follower Unbound gibi tüketici modlara iletir; tüketici bu öğeleri normal eşya toplama, satma veya ekipman yönetimi sırasında koruyabilir. Tedavi öğesinin kaydı, King Core'un takipçiyi otomatik tedavi ettiği veya iksir satın aldığı anlamına gelmez; tedavi davranışını ilgili modlar yürütür.

Follower Unbound oyuncunun uyku veya beklemesi sona erdiğinde aktif takipçilerini dinlenmiş sayar ve oyun günü bilgisini King Core'a yollar. King Core bu dinlenme olayını Life Again'e iletir; Life Again hafif yara iyileşme ilerlemesini günceller. Böylece Follower Unbound'un mevcut uyku davranışı Life Again'in yara toparlanma sistemiyle ortak bir olay üzerinden çalışır.

### SoulKeeper

SoulKeeper, soul gem yönetimini üstlendiğini King Core'a bildirir. Bildirim etkin olduğunda tüketici mod, kendi soul gem satın alma/toplama sistemini geri çekip yönetimi SoulKeeper'a bırakabilir.

> Bu entegrasyonlar SKSE mesajlaşması ve King Core API protokolü üzerinden kurulur. Uyumlu modların ilgili King Core entegrasyonunu içeren sürümleri kurulu olmalıdır.

## PrismaUI durum menüsü

PrismaUI kuruluysa **Shift + K** ile King Core paneli açılır. Panel ClassBound, Follower Unbound, SoulKeeper ve Life Again bağlantı durumlarını yeşil tik veya kırmızı çarpı ile gösterir. Bir modun işaretinin etkin olması, King Core'un o moddan beklenen kayıt/istek mesajını aldığını gösterir; tek başına modun tüm oyun içi özelliklerinin çalıştığını doğrulamaz.

## Kurulum

1. Skyrim Special Edition/AE için SKSE ve CommonLibSSE-NG ile uyumlu King Core DLL'sini kurun.
2. DLL'yi `Data/SKSE/Plugins` altına yerleştirin.
3. PrismaUI durum menüsünü kullanacaksanız PrismaUI'yi kurun ve `Data/PrismaUI/views/KingCore/index.html` dosyasını paket içindeki konumunda tutun.
4. Entegrasyon için kullanacağınız ClassBound, Follower Unbound, Life Again ve/veya SoulKeeper modlarının King Core destekli sürümlerini kurun.

## Geliştirici notu

Paylaşılan API tanımları `include/KingCore/KingCoreAPI.h` dosyasındadır. API ana sürümü 1'dir. Sınıf API'si, sınıf bilgisi ve davranış profili sorgularını; mesaj protokolü ise tedavi öğesi, soul gem yöneticisi kaydı ve takipçi dinlenme olayını tanımlar. API ana sürümü farklı olan sağlayıcılar güvenlik için reddedilir.

Derleme CMake ve CommonLibSSE-NG kullanır:

```powershell
cmake -S . -B build
cmake --build build --config Release --target KingCore
```

