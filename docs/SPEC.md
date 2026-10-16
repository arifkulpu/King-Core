FOLLOWER UNBOUND
GELİŞTİRME VE HATA DÜZELTME SPESİFİKASYONU (v2)

Not: [YENİ] ve [GÜNCELLENDİ] etiketleri, v1'e göre eklenen veya değiştirilen yerleri gösterir.

============================================================
1. GENEL AMAÇ
============================================================

Follower Unbound takipçi yapay zekâsını daha güvenilir, doğal ve modüler hale getir.

Bu çalışma aşağıdaki sistemleri kapsar:

- Classbound sınıf entegrasyonu
- King Core entegrasyonu
- Yer ganimetlerini toplama
- Birden fazla takipçi arasında ganimet dağıtımı
- Takipçi ekipmanlarını koruma
- Slot tabanlı Protected Equipment sistemi
- Akıllı satış ve alış sistemi
- Yemek ve içecek sistemi
- SunHelm uyumluluğu
- Çoklu takipçi ihtiyaç kuyruğu
- İksir kullanım sistemi
- Kritik durumlarda acil iksir kullanımı
- Takipçi hareket hızı problemi
- Teleport ve uyanma sonrası stabilizasyon
- Hücre/worldspace geçişleri
- Loading Screen sırasında AI işlemlerinin durdurulması
- Takipçi envanteri oyuncu tarafından açıldığında otomatik işlemlerin durdurulması
- Merkezi State / Action Queue sistemi
- Performans sınırları [YENİ]
- Debug ve hata ayıklama sistemi

Ana hedef:

Takipçi kendi ihtiyaçlarını, envanterini, ticaretini ve ganimetini yönetebilmeli; ancak loading, transition, teleport, oyuncu envanter kontrolü veya kritik oyun durumlarında işlemlerini güvenli biçimde durdurmalıdır.


============================================================
2. KRİTİK MİMARİ KURALI — CLASSBOUND / KING CORE
============================================================

DOĞRU VERİ AKIŞI:

Classbound
    ↓
King Core
    ↓
Follower Unbound

Follower Unbound Classbound DLL/API'sini doğrudan sorgulamamalıdır.

Classbound tarafından belirlenen sınıf bilgisi King Core üzerinden alınmalıdır.


2.1 SINIF ÖNCELİĞİ

Sınıf belirleme sırası:

1. Classbound sınıfı → King Core üzerinden
2. King Core tarafından sağlanan diğer geçerli sınıf bilgileri
3. Follower Unbound fallback sınıf algılama sistemi

Örnek:

Classbound = Elementalist
King Core = Elementalist
Follower Unbound fallback = Mage / Spellcaster

Final Class = Elementalist

Fallback sistemi King Core bilgisini geçersiz kılamamalıdır.


2.2 SINIF DEĞİŞİKLİĞİ

Oyuncu Classbound üzerinden takipçinin sınıfını değiştirirse eski cache kullanılmamalıdır.

King Core'dan güncel sınıf yeniden alınmalıdır.

Debug:

[ClassDetection]
[KingCore]
[FollowerUnbound]

Örnek:

[ClassDetection] Requesting follower class.
[KingCore] Class returned: Elementalist.
[FollowerUnbound] Final class: Elementalist.


2.3 KING CORE ERİŞİLEMEZ / BOŞ SONUÇ [YENİ]

Durumlar ve davranış:

A) King Core yüklü değil:
→ Fallback sınıf algılama kullanılır.

B) King Core yüklü, ama takipçi için sınıf döndürmedi (boş / None / geçersiz):
→ Fallback sınıf algılama kullanılır.
→ Sonuç "geçici" olarak işaretlenir.
→ Bir sonraki cell yüklemesinde veya sınıf değişikliği olayında King Core yeniden sorgulanır.

C) King Core geçerli sınıf döndürdü:
→ Fallback asla devreye girmez, King Core sonucu geçerlidir.

D) King Core çağrısı hata verdi veya zaman aşımına uğradı:
→ Crash olmamalı.
→ Son bilinen geçerli King Core sınıfı (varsa) kullanılır.
→ Yoksa fallback kullanılır.
→ Hata loglanır.

Debug:

[KingCore] Returned empty class for follower: Lydia.
[ClassDetection] Using fallback class: Mage (temporary).
[KingCore] Will re-query on next cell load.

Sınıf bilgisi hiçbir durumda iksir, loot veya ticaret sistemini tamamen kilitlememelidir.


============================================================
3. YER GANİMETİ TOPLAMA
============================================================

Takipçilerin yerde bulunan ganimetleri toplama sistemi tekrar aktif ve güvenilir hale getirilmelidir.

Toplanabilecek örnekler:

- Silah
- Zırh
- Kask
- Eldiven
- Bot
- Ok
- Bolt
- İksir
- Yiyecek
- İçilebilir eşyalar
- Altın
- Diğer satılabilir ganimetler

Loot toplama tamamen sınıfa göre engellenmemelidir.

Örneğin Archer yalnızca ok, Warrior yalnızca silah toplamak zorunda değildir.

Takipçi eşyayı daha sonra satabilecekse gerekli ganimeti toplayabilmelidir.


============================================================
4. ÇOKLU TAKİPÇİ GANİMET DAĞITIMI
============================================================

Birden fazla takipçi varsa tüm ganimet tek takipçiye verilmemelidir.

Örnek:

3 takipçi:

Follower 1
Follower 2
Follower 3

NPC ganimeti:

1 Helmet
1 Gloves
1 Boots
3 Potions
2 Weapons

Beklenen dağılım:

Follower 1:
Helmet
Potion
Weapon

Follower 2:
Gloves
Potion
Weapon

Follower 3:
Boots
Potion

Round-robin:

Item 1 → Follower 1
Item 2 → Follower 2
Item 3 → Follower 3
Item 4 → Follower 1
Item 5 → Follower 2
Item 6 → Follower 3
...

Takipçi ve item sayısı değişse bile sistem çalışmalıdır.

Stack miktarları mümkün olduğunda bulk/stack işlemle işlenmelidir.


4.1 ROUND-ROBIN SIRA DURUMU [YENİ]

Round-robin sayacı her loot olayında sıfırlanmamalıdır.

Doğru davranış:

Sıra kaldığı yerden devam eder.

Örnek:

Loot olayı 1: 2 item → Follower 1, Follower 2
Loot olayı 2: 2 item → Follower 3, Follower 1

Yanlış davranış:

Her olayda Follower 1'den başlamak (uzun vadede Follower 1'i kayırır).

Kurallar:

- Sıra sayacı kalıcı olmalı (save/load sonrası da korunmalı veya en kötü ihtimalle güvenli şekilde sıfırlanmalı).
- Takipçi gruptan çıkarsa veya eklenirse sayaç geçerli aralığa normalize edilmeli.
- Ölü, devre dışı veya erişilemeyen takipçi atlanmalı; item bir sonraki uygun takipçiye verilmeli.
- Bir takipçinin taşıma kapasitesi doluysa atlanmalı.

Debug:

[LootDistribution] Next follower index: 2 (Follower 3).
[LootDistribution] Follower 2 unavailable, skipped.


============================================================
5. PROTECTED EQUIPMENT — GENEL AMAÇ
============================================================

Takipçilerin kendi kıyafetlerini veya oyuncunun onlara verdiği ekipmanı yanlışlıkla satması engellenmelidir.

Problem:

Follower başlangıç kıyafetini giyiyor
    ↓
Trade sistemi eşyayı satılabilir görüyor
    ↓
Follower kıyafetini satıyor
    ↓
Follower çıplak kalıyor

Protected Equipment sistemi oyuncunun belirlediği mevcut ekipmanı korumalıdır.

ÖNEMLİ:

Koruma sistemi genel loot sistemini kapatmamalıdır.

Takipçinin kendi korunan eşyası satılamaz.
Düşmandan alınan aynı türde başka eşya ise normal şekilde toplanabilir ve satılabilir.


============================================================
6. PROTECTED EQUIPMENT SLOT SİSTEMİ
============================================================

Sistem yalnızca temel zırh slotlarını değil, modlu ekipmanların kullandığı ilgili slotları da desteklemelidir.


6.1 TEMEL SLOT'LAR

Slot 30 - Head:
Miğferler, şapkalar ve kapüşonlar.

Slot 31 - Hair:
Bazı açık şapkalar ve saç modelini gizlemeden kullanılan baş ekipmanları.

Slot 32 - Body / Cuirass:
Tüm ana zırhlar, kıyafetler ve cübbeler.

Slot 33 - Hands:
Eldivenler ve kolluklar.

Slot 37 - Feet:
Çizmeler ve ayakkabılar.

Slot 42 - Circlet:
Cevherli taçlar / Circlet.

Slot 55 - Amulet:
Tılsımlar ve kolyeler.

Slot 56 - Ring:
Yüzükler.


6.2 GENİŞLETİLMİŞ MOD SLOT'LARI

Slot 40 - Tail:
Kuyruk ve bazı ırk/mod aksesuarları.

Slot 41 - Long Hair:
Uzun saç / ekstra başlık detayları.

Slot 43 - Ears:
Küpeler ve kulak aksesuarları.

Slot 44 - Face / Mask:
Yüz maskeleri ve benzeri yüz ekipmanları.

Slot 45 - Neck / Muffler:
Boyunluklar ve atkılar.

Slot 46 - Cloak / Cape:
Pelerin ve cape ekipmanları.

Slot 47 - Backpack:
Sırt çantaları.

Slot 49 - Skirt / Girdle:
Etekler ve kalça üstü kemer aksesuarları.

Slot 52 - Undergarment / Pants:
İç çamaşırı, pantolon ve alt gövde ekipmanları.

Slot 57 - Pauldrons:
Omuzluklar.

Slot 59 - Piercings / ShoulderR:
Piercing ve sağ omuz detayları.

Slot 60 - Belt:
Kemerler, utility belt ve taşınabilir fener gibi ekipmanlar.


6.3 MERKEZİ SLOT LİSTESİ

Slotlar mümkün olduğunca merkezi bir yapıdan yönetilmelidir.

Örnek:

ProtectedEquipmentSlots =

30
31
32
33
37
40
41
42
43
44
45
46
47
49
52
55
56
57
59
60

Yeni modlu bir slot gerektiğinde bütün sistemi değiştirmek yerine listeye eklenebilmelidir.


============================================================
7. RESET PROTECTED EQUIPMENT
============================================================

Ayar:

Reset Protected Equipment

Kullanım:

Takipçiyi seç
    ↓
İstediğin ekipmanları giydir
    ↓
Reset Protected Equipment
    ↓
Mevcut ekipmanlar korumalı olarak kaydedilir

Örnek:

Slot 30 → Hood
Slot 32 → Robe
Slot 33 → Gloves
Slot 37 → Boots
Slot 46 → Cloak
Slot 47 → Backpack
Slot 60 → Belt

Bunların tamamı korunmalıdır.


7.1 BOŞ SLOT

Slot boşsa koruma kaydı oluşturulmasına gerek yoktur.

Örneğin:

Slot 42 → Empty
Slot 55 → Amulet
Slot 56 → Ring

Sonuç:

Slot 42 → Nothing to protect
Slot 55 → Protect current Amulet
Slot 56 → Protect current Ring


============================================================
8. SLOT DEĞİL, BELİRLİ EKİPMAN KORUNMALI
============================================================

Sistem yalnızca:

"Slot 32 korunuyor"

mantığıyla çalışmamalıdır.

Doğru mantık:

"Slot 32'de şu anda bulunan bu belirli ekipmanı koru."

Örnek:

İlk kayıt:
Slot 32 → Mage Robe

Oyuncu:
Mage Robe çıkarır
Heavy Armor giyer

Reset yapılmadıysa:

Mage Robe → Protected
Heavy Armor → Normal

Reset yapılırsa:

Mage Robe → eski korumadan çıkar
Heavy Armor → Protected

Bu davranış oyuncuya tam kontrol sağlar.


============================================================
9. FORM ID / ITEM KİMLİĞİ
============================================================

Koruma sistemi mümkün olduğunca gerçek ekipman kimliğini kaydetmelidir.

Mantık:

Follower
    ↓
Protected Slot
    ↓
Equipped Item
    ↓
FormID / güvenli item referansı

Modlu ekipman ve farklı instance durumları güvenli biçimde ele alınmalıdır.

Eksik veya geçersiz kayıt crash oluşturmamalıdır.


9.1 AYNI FORMID'DEN BİRDEN FAZLA KOPYA [YENİ]

Sorun:

Takipçide 2 adet Iron Armor var.
Biri giyili ve korumalı, diğeri envanterde.
Yalnızca FormID'ye bakılırsa ikisi de korumalı sanılabilir veya yanlışlıkla giyili olan satılabilir.

Kural:

1. Koruma kaydı FormID + miktar mantığıyla çalışır:
   Protected Count = Reset anında giyili olan kopya sayısı (normalde 1).

2. Aynı FormID'den korunan sayıdan FAZLA kopya varsa, fazlası normal eşya sayılır ve satılabilir.

3. Satış sırasında korunan kopya sayısı envanterden düşülerek satılabilir miktar hesaplanır:
   Satılabilir = Toplam - Protected Count

   Örnek:
   Iron Armor toplam = 3, Protected Count = 1 → En fazla 2 satılabilir.

4. Giyili olan kopya her zaman korunan kopya olarak kabul edilir.

5. Modlu / enchant edilmiş / temperlenmiş / özel isimli eşyalar için mümkünse instance verisi (ExtraData, unique ID veya eşdeğeri) de kaydedilmelidir.
   - Instance verisi alınamıyorsa FormID + miktar yöntemine güvenli şekilde geri dönülmelidir.
   - Bu durumda belirsizlik "koruma lehine" çözülmelidir (şüphede satma).

6. Takipçi korunan eşyayı çıkarır veya oyuncu envanterden alırsa:
   - Kayıt silinmez (Reset yapılmadıkça korunma devam eder).
   - Eşya envanterde yoksa kayıt "eksik" olarak işaretlenir, crash oluşturmaz.

Debug:

[EquipmentProtection]
Item: Iron Armor
Total: 3
Protected Count: 1
Sellable: 2


============================================================
10. SATIŞ SİSTEMİYLE ENTEGRASYON
============================================================

Trade/Sell sistemi herhangi bir eşyanın satılıp satılamayacağını kontrol ederken:

IsProtectedEquipment(item)?

kontrolü yapmalıdır.

TRUE:
→ Satma

FALSE:
→ Normal satış kurallarına devam et

(Miktar bazlı kontrol için bölüm 9.1 geçerlidir.)


Örnek:

[EquipmentProtection]
Item: Mage Robe
Slot: 32
Status: PROTECTED
Action: SELL BLOCKED

Korumasız eşya:

[EquipmentProtection]
Item: Iron Sword
Status: NOT PROTECTED
Action: SELL ALLOWED


============================================================
11. LOOT SİSTEMİYLE ENTEGRASYON
============================================================

Protected Equipment sistemi loot toplamayı engellememelidir.

Örneğin:

Takipçinin üzerinde:
Protected Black Mage Robe

Düşmandan:
Iron Armor
Steel Boots
Leather Gloves
Cloak

Takipçi bunları yine toplayabilir.

Sonrasında:

Enemy loot → Satılabilir
Protected Black Mage Robe → Satılamaz


============================================================
12. EQUIPMENT DEĞİŞTİRME
============================================================

Follower Unbound ileride otomatik ekipman değiştirme sistemi kullanırsa Protected Equipment bununla uyumlu olmalıdır.

Örneğin:

Protected:
Iron Armor

Takipçi daha iyi Steel Armor bulur.

Equipment sistemi Steel Armor'u giydirirse:

Iron Armor → hâlâ Protected
Steel Armor → normal

Reset Protected Equipment yapılırsa:

Steel Armor → Protected

Varsayılan davranışta sistem oyuncunun koruma tercihini otomatik değiştirmemelidir.


============================================================
13. SAVE / LOAD
============================================================

Protected Equipment bilgileri save/load sonrasında korunmalıdır.

Oyun kapatılıp açıldığında:

Follower
    ↓
Protected Equipment Data
    ↓
Restore

Eksik veya artık bulunamayan item referansları crash oluşturmadan güvenli şekilde temizlenebilir.


============================================================
14. TAKİPÇİ BAZLI KORUMA
============================================================

Her takipçinin koruma listesi ayrı olmalıdır.

Follower A:
Protected Equipment Set A

Follower B:
Protected Equipment Set B

Follower C:
Protected Equipment Set C

Bir takipçinin Reset işlemi diğerlerini etkilememelidir.


============================================================
15. AKILLI SATIŞ
============================================================

Takipçi satıcıya gittiğinde:

1. Protected Equipment kontrol et.
2. İhtiyaç için gerekli stokları koru.
3. Gereksiz eşyaları belirle.
4. Gereksiz eşyaları sat.

Satılmaması gerekenler:

- Protected Equipment
- Minimum stok için gerekli eşyalar
- İhtiyaç için ayrılmış eşyalar

Satılabilecekler:

- Fazla silah
- Fazla zırh
- Fazla yiyecek
- Fazla içecek
- Fazla iksir
- Fazla mühimmat
- Gereksiz ganimet


============================================================
16. AKILLI SATIN ALMA
============================================================

Alışveriş önceliği:

1. Food
2. Drink / Water
3. Health Potion
4. Magicka Potion
5. Stamina Potion
6. Arrows / Bolts

Örnek:

Target Food = 10
Current Food = 3

Purchase = 7

Takipçi 10 tane daha satın alıp 13'e çıkmamalıdır.


============================================================
17. OK / BOLT
============================================================

Takipçinin silah türüne göre mühimmat belirlenmelidir.

Archer:
→ Arrows

Crossbow:
→ Bolts

Takipçi gereksiz yanlış mühimmat satın almamalıdır.


============================================================
18. BULK TRADE
============================================================

v0.1.5 ile gelen toplu ticaret sistemi korunmalıdır.

Desteklenen çarpanlar:

1x
3x
5x
10x

Amaç:

Her potion, food veya arrow için ayrı transaction çalıştırmak yerine mümkün olduğunca toplu işlem kullanmak.

Sistem:

- Performansı artırmalı
- İşlem hızını artırmalı
- Fazla eşya satışı problemini çözmeli
- Gold senkronizasyonunu doğru yapmalı
- Inventory refresh işlemini güvenilir yapmalı
- Transaction timing problemlerini önlemeli


============================================================
19. SUNHELM İÇECEK DESTEĞİ
============================================================

SunHelm kuruluysa Follower Unbound geçerli SunHelm içeceklerini algılamalıdır.

Örnekler:

- Water
- Waterskin
- Milk
- Wine
- SunHelm su/içecek eşyaları
- Diğer geçerli SunHelm içecekleri

Takipçi içecek ihtiyacını karşılamak için bunları satın alabilmelidir.

SunHelm kurulu değilse Follower Unbound kendi standart sistemini kullanmalıdır.


============================================================
20. YEMEK / İÇECEK ACTION QUEUE
============================================================

Birden fazla takipçi aynı anda yemek veya içmek yapmamalıdır.

Merkezi kuyruk:

Follower 1
    ↓
Drink
    ↓
3 saniye
    ↓
Food
    ↓
Finish

Follower 2
    ↓
Drink
    ↓
3 saniye
    ↓
Food
    ↓
Finish

Follower 3
    ↓
Drink
    ↓
3 saniye
    ↓
Food
    ↓
Finish

Follower 2, Follower 1 tamamen bitmeden başlamamalıdır.


============================================================
21. DRINK ÖNCELİĞİ
============================================================

Takipçinin hem yemek hem içmek ihtiyacı varsa:

Drink
    ↓
3 saniye
    ↓
Food

İki işlem aynı anda yapılmamalıdır.


============================================================
22. İHTİYAÇ HEDEFİ
============================================================

Takipçi ayarlanan hedefe ulaşmaya çalışmalıdır.

Örnek:

Food Target = 100%
Drink Target = 100%

Ancak:

- Sonsuz döngü olmamalı
- Inventory'de eşya yoksa sürekli denememeli
- Satın alma başarısızsa state doğru güncellenmeli
- Animation başarısız olduğunda sistem kilitlenmemeli
- Tamamlanan işlem tekrar edilmemeli


============================================================
23. SUNHELM ANİMASYON FALLBACK
============================================================

Normal Follower Unbound yemek/içmek animasyonu başarısız veya kullanılamazsa sistem tamamen durmamalıdır.

SunHelm kuruluysa fallback kullanılmalıdır.

Yemek:

Follower Unbound animation
    ↓ başarısız
SunHelm bread-eating animation

İçmek:

Follower Unbound animation
    ↓ başarısız
SunHelm drinking/water animation

Needs sistemi animation başarısızlığı nedeniyle kilitlenmemelidir.


============================================================
24. TAKİPÇİ HAREKET HIZI PROBLEMİ [GÜNCELLENDİ]
============================================================

Bazı durumlarda takipçi aşırı yavaş hareket ediyor:

Yaklaşık 1 metre / 10 saniye.

Oyunu yeniden başlatmak problemi geçici olarak düzeltebiliyor.

Kök neden bilinmiyor. Bu yüzden ÖNCE teşhis altyapısı yazılmalı, düzeltme sonra yapılmalıdır.
Bu madde, uygulama sırasında HER ŞEYDEN ÖNCE ele alınacak ilk görevdir (bkz. bölüm 45, Faz 0).


24.1 ARAŞTIRILACAK ALANLAR

- Movement Speed
- SpeedMult
- Actor Values
- AI Package
- Follow Package
- Combat Package
- Animation State
- Navigation
- Wait State
- Sleep State
- Eating State
- Drinking State
- Potion State
- Temporary Actor Modifiers
- Package Override
- Movement restoration

Kod içinde özellikle araştır:

SetSpeedMult
SpeedMult
Movement
AI Package
Package Override
Actor Value


24.2 SOMUT GÖREVLER [YENİ]

1. Snapshot / Restore:
   Follower Unbound bir aksiyon başlatmadan ÖNCE takipçinin hız ile ilgili değerlerini (SpeedMult vb.) kaydeder.
   Aksiyon bittiğinde, iptal edildiğinde, başarısız olduğunda veya kesildiğinde (loading, teleport, ölüm vb.) bu değerler MUTLAKA geri yüklenir.

2. Sapma kontrolü:
   Her aksiyon sonrası SpeedMult beklenen değerle karşılaştırılır.
   Sapma varsa:
   - Loglanır (hangi aksiyondan sonra olduğu ile birlikte)
   - Kaydedilen değere geri düzeltilir

3. Periyodik doğrulama:
   Takipçi "Following" state'indeyken belirli aralıklarla SpeedMult kontrol edilir.
   Anormal düşük değer tespit edilirse düzeltilir ve loglanır.

4. Geri yükleme garantisi:
   Aksiyonun herhangi bir noktada yarıda kesilmesi durumunda (hata, loading, cell değişimi) restore işlemi finally-benzeri güvenli bir yolda çalışmalıdır.

5. Yeniden üretim (repro) logu:
   Hangi aksiyon dizisinden sonra yavaşlama başladığını yakalamak için aksiyon geçmişi (son N aksiyon) loglanır.

Follower Unbound hiçbir durumda hareket hızını kalıcı olarak değiştirmemelidir.

Geçici değişiklik varsa eski değer mutlaka geri yüklenmelidir.


============================================================
25. MOVEMENT DEBUG
============================================================

Örnek:

[Movement]
Follower: Lydia
SpeedMult: 100
State: Following
Package: FollowPlayer

Anormal:

[Movement]
Follower: Lydia
SpeedMult: 10
State: Eating

Amaç, yavaşlamanın hangi sistem tarafından oluşturulduğunu bulmaktır.

Ek log örnekleri [YENİ]:

[Movement] Snapshot saved before Drink. SpeedMult: 100.
[Movement] Restore after Drink. SpeedMult: 100 -> 100. OK.
[Movement] DEVIATION after Food: expected 100, found 10. Corrected.
[Movement] Last actions: Loot, Sell, Drink, Food.


============================================================
26. İKSİR SİSTEMİ SINIFTAN BAĞIMSIZ
============================================================

İksir kullanımı Classbound sınıfına bağlı olmamalıdır.

Her takipçi ihtiyaç yüzdesine göre iksir kullanabilir.

Örnek:

Health < 50%
→ Health Potion

Magicka < 40%
→ Magicka Potion

Stamina < 30%
→ Stamina Potion

Class, iksir kullanımını tamamen engellememelidir.


============================================================
27. KRİTİK / ACİL İKSİR
============================================================

Normal iksir sisteminden ayrı acil durum katmanı olmalıdır.

Örnek:

Health < 15%
→ Immediate Health Potion

Magicka < 5%
→ Immediate Magicka Potion

Stamina < 5%
→ Immediate Stamina Potion

Critical potion kullanımı normal cooldown'ı beklememelidir.


27.1 KRİTİK İKSİR İSTİSNA KURALLARI [YENİ]

Critical Survival, oyuncu envanter kontrolünü ve stabilizasyon beklemelerini aşabilir; ancak LOADING / CELL TRANSITION sırasında çalışamaz.

Durum tablosu:

Normal oyun:
→ Critical potion çalışır.

Oyuncu takipçi envanterini açmış:
→ Critical potion ÇALIŞIR (hayatta kalma önceliklidir).
→ Satma, alma, equip, loot gibi diğer otomatik işlemler askıda kalır.
→ İksir önce takipçi envanterinde mevcut olmalıdır; envanter açıkken oyuncunun o anda ekranda değiştirdiği eşya güvenli biçimde okunmalıdır.

Teleport / Wake-up stabilizasyonu:
→ Critical potion ÇALIŞIR (kısa ve güvenli bir kontrolle).
→ Diğer tüm otomatik işlemler beklemede kalır.

Loading Screen / Cell Transition:
→ Critical potion ÇALIŞMAZ (oyun state'i güvensiz).
→ Health kritikse, loading bittikten sonra stabilizasyon beklemesi ATLANARAK ilk güvenli anda kritik iksir kullanılır.

Debug:

[CriticalPotion]
Health = 10%. Loading active. Potion deferred.

[CriticalPotion]
Loading finished. Stabilization bypassed for critical survival.
Using Health Potion immediately.


============================================================
28. İKSİR ÖNCELİĞİ
============================================================

Öncelik:

CRITICAL SURVIVAL
    ↓
NORMAL POTION
    ↓
OTHER AUTONOMOUS ACTIONS

Örneğin takipçi yemek kuyruğundayken Health = 10% olursa:

Immediate Health Potion

Debug:

[CriticalPotion]
Health = 10%
Emergency threshold = 15%
Using Health Potion immediately.

Not: Loading/transition sırasındaki istisna için bölüm 27.1 geçerlidir.


============================================================
29. MERKEZİ FOLLOWER STATE / ACTION QUEUE
============================================================

Bütün sistemler ayrı ayrı timer kullanmamalıdır.

Önerilen yapı:

Follower State
    ↓
Transition Manager
    ↓
Action Queue
    ↓
Needs / Inventory / Trading / Loot / Potion / Equipment

Sistemler birbirleriyle çakışmadan merkezi state üzerinden çalışmalıdır.

Action Queue ve Transition Manager AYNI ANDA tasarlanmalıdır; biri diğerinden sonra eklenirse refactor gerekir. [YENİ]


29.1 PERFORMANS SINIRLARI [YENİ]

Çok takipçi + bulk işlem + merkezi queue birlikte çalışırken performans korunmalıdır.

Sınırlar (başlangıç değerleri, ayarlanabilir):

- Maksimum kuyruk uzunluğu: takipçi başına 10, toplam 40 aksiyon
  (aşılırsa en düşük öncelikli ve en eski aksiyon düşürülür, loglanır)

- Frame / tick başına işlem limiti: en fazla 1 ağır aksiyon
  (trade, bulk loot, inventory işlemi ağır sayılır)

- Aynı aksiyon tekrar kuyruğa eklenmeden önce kopya kontrolü yapılır

- Uzun süre (örn. 30 sn) ilerlemeyen aksiyon zaman aşımıyla iptal edilir ve state sıfırlanır
  (kilitlenme koruması)

- Kuyruk boşken sistem gereksiz polling yapmamalıdır; olay tabanlı tetikleme tercih edilir.

Debug:

[ActionQueue] Queue length: 7 / 40.
[ActionQueue] Dropped low-priority action: Loot (Follower 3).
[ActionQueue] Action timeout: Drink (Follower 2). State reset.


============================================================
30. GENEL EYLEM ÖNCELİĞİ [GÜNCELLENDİ]
============================================================

1. LOADING / CELL TRANSITION
   (Critical potion dahil hiçbir şey çalışmaz; loading sonrası critical potion için stabilizasyon atlanır.)
    ↓
2. CRITICAL SURVIVAL
   (Oyuncu envanteri açıkken ve teleport/wake stabilizasyonunda da çalışabilir.)
    ↓
3. PLAYER INVENTORY INTERACTION
    ↓
4. TELEPORT / POSITIONING
    ↓
5. FOLLOWER ACTION QUEUE
    ↓
6. NORMAL AI

Açıklama:

- Loading ve cell transition en üstte, çünkü oyun state'i güvenli değil.
- Critical Survival bunun hemen altında: takipçinin ölümünü önleyen tek otomatik aksiyon sınıfıdır.
- Oyuncu envanter kontrolü, normal otomatik aksiyonların hepsinin üstündedir.
- Oyuncu envanteri açıkken Critical Survival çalışabilir ama satma/alma/equip/loot gibi işlemler çalışmaz.

Bu sıralama, bölüm 41 ve bölüm 49 ile tutarlıdır.


============================================================
31. FOLLOWER TRANSITION & ACTION DELAY
============================================================

Takipçi teleport olduğunda, uyandığında veya yeni cell/worldspace'e geçtiğinde hemen başka autonomous action başlatmamalıdır.

Kısa stabilizasyon süresi uygulanmalıdır.

Amaç:

- AI çakışmalarını önlemek
- Teleport sonrası yanlış inventory işlemlerini önlemek
- Loading sonrası erken trade işlemlerini engellemek
- Animation ve package state'lerinin oturmasını beklemek
- Takipçinin doğal davranmasını sağlamak

Transition sistemi merkezi Action Queue ile entegre olmalıdır.


============================================================
32. TELEPORT SONRASI GECİKME
============================================================

Teleport:

Teleport
    ↓
Position stabilized
    ↓
2–3 saniye stabilizasyon
    ↓
AI resume

Teleporttan hemen sonra:

- Loot
- Trade
- Buy
- Sell
- Eat
- Drink
- Inventory processing
- Equipment processing

başlatılmamalıdır.

Gecikme ayarlanabilir olmalıdır.

İstisna: Critical Survival (bölüm 27.1).


============================================================
33. UYKUDAN UYANMA
============================================================

Wake:

Wake
    ↓
Wake animation/state
    ↓
2–3 saniye stabilizasyon
    ↓
Normal AI

Uyanır uyanmaz:

- Yemek
- İçmek
- Ticaret
- Loot
- Inventory processing
- Teleport

başlatılmamalıdır.

İstisna: Critical Survival (bölüm 27.1).


============================================================
34. OYUNCU TAKİPÇİ ENVANTERİNİ AÇTIĞINDA
============================================================

Player follower inventory açtığında:

PLAYER INVENTORY INTERACTION

state'i aktif olmalıdır.

Aşağıdaki işlemler askıya alınmalıdır:

- Sell
- Buy
- Equip
- Unequip
- Loot processing
- Inventory processing
- Automatic consumption (normal yemek/içmek/normal iksir)
- Protected equipment recalculation

İstisna [YENİ]:

- Critical Survival (acil iksir) envanter açıkken de çalışır.

Amaç:

Oyuncunun takipçiye verdiği bir eşyanın takipçi tarafından hemen satılmasını engellemek.


============================================================
35. ENVANTER KAPANDIĞINDA
============================================================

Inventory Closed
    ↓
1–2 saniye bekle
    ↓
Inventory yeniden değerlendir
    ↓
Needs yeniden değerlendir
    ↓
Normal AI

Takipçi oyuncunun yaptığı değişiklikleri önce algılamalıdır.


============================================================
36. ŞEHİR / BİNA / CAVE / DUNGEON GEÇİŞİ
============================================================

Yeni bölgeye giriş:

Cell Transition Detected
    ↓
Suspend AI
    ↓
Loading
    ↓
Cell Loaded
    ↓
Stabilization Delay
    ↓
Resume AI

Desteklenmeli:

- Şehir
- Kasaba
- Ev
- Bina
- Mağara
- Zindan
- Maden
- Harabe
- Interior
- Exterior
- Worldspace değişimi
- Diğer cell geçişleri


============================================================
37. LOADING SCREEN KORUMASI
============================================================

Loading Screen sırasında takipçi autonomous action çalıştırmamalıdır.

Özellikle:

- Loot
- Trade
- Buy
- Sell
- Eat
- Drink
- Inventory processing
- Equipment processing
- Class processing
- Teleport logic
- Normal AI actions
- Critical potion (loading bitene kadar ertelenir) [YENİ]

çalıştırılmamalıdır.

İç state:

Loading

Loading bittikten sonra:

Loading
    ↓
Stabilizing
    ↓
Ready


============================================================
38. LOADING SONRASI STABİLİZASYON
============================================================

Loading tamamlandığı anda bütün sistemler aynı frame'de çalıştırılmamalıdır.

Örnek:

Cell Loaded
    ↓
2–3 saniye stabilizasyon
    ↓
Follower Ready
    ↓
Action Queue Resume

İstisna: Critical Survival gerekiyorsa stabilizasyon atlanır (bölüm 27.1).


============================================================
39. ACTION QUEUE + TRANSITION
============================================================

Örneğin takipçi yemek yiyor:

Drink
    ↓
Transition detected
    ↓
Action Queue pause
    ↓
Loading
    ↓
Cell Loaded
    ↓
Stabilization
    ↓
Needs re-evaluate
    ↓
Action Queue resume

Tamamlanmış işlem tekrar yapılmamalıdır.

Örneğin takipçi içmeyi tamamladıysa loading sonrası tekrar içmemelidir.

Yarıda kesilen aksiyonlarda hız değerleri geri yüklenmelidir (bölüm 24.2). [YENİ]


============================================================
40. TİCARET + TRANSITION KORUMASI
============================================================

Takipçi merchant ile işlem yaparken cell transition gerçekleşirse:

Trade
    ↓
Transition detected
    ↓
Safely stop/pause transaction
    ↓
Loading
    ↓
New cell ready
    ↓
Inventory re-check
    ↓
Trade yeniden değerlendirilir

Loading sırasında merchant transaction devam ettirilmemelidir.

Gold ve inventory state yeniden senkronize edilmelidir.


============================================================
41. OYUNCU KONTROLÜ ÖNCELİĞİ [GÜNCELLENDİ]
============================================================

Temel kural:

PLAYER CONTROL
    >
FOLLOWER AUTONOMOUS ACTIONS

Oyuncu takipçinin envanterini kontrol ediyorsa takipçi kendi trade/inventory sistemini çalıştırmamalıdır.

Oyuncu kontrolü sona erdiğinde kısa gecikme uygulanmalı ve sistem yeniden değerlendirilmelidir.

Tek istisna:

CRITICAL SURVIVAL (acil iksir). Takipçinin ölümünü önlemek, oyuncu envanter kontrolünün üzerinde tutulur. Loading/transition sırasında ise bu da ertelenir.


============================================================
42. AYARLANABİLİR GECİKMELER [GÜNCELLENDİ]
============================================================

Başlangıç değerleri:

Teleport Stabilization:
2–3 saniye

Wake-Up Stabilization:
2–3 saniye

Inventory Interaction:
1–2 saniye

Cell Transition:
2–3 saniye

Post-Merchant Interaction:
1–2 saniye

Değerler MCM veya config üzerinden ayarlanabilir hale getirilebilir.

Varsayılan değerler gereksiz uzun bekleme oluşturmamalıdır.


42.1 ALT / ÜST SINIRLAR [YENİ]

Kullanıcı tanımlı değerler sabit sınırlar içine sıkıştırılmalıdır (clamp). Sınır dışı değer girilirse en yakın geçerli değere çekilir ve loglanır.

Teleport Stabilization:        min 1 sn  — max 10 sn
Wake-Up Stabilization:         min 1 sn  — max 10 sn
Inventory Interaction:         min 0.5 sn — max 5 sn
Cell Transition:               min 1 sn  — max 10 sn
Post-Merchant Interaction:     min 0.5 sn — max 5 sn
Drink → Food arası bekleme:    min 1 sn  — max 10 sn

Amaç:

Kullanıcının 0 saniye (çakışma ve hata riski) veya 60 saniye (takipçi donmuş gibi davranır) girerek sistemi bozmasını önlemek.

Debug:

[Transition] Config value 0s below minimum. Clamped to 1s.


============================================================
43. DEBUG LOG KATEGORİLERİ
============================================================

[ClassDetection]
[KingCore]
[EquipmentProtection]
[LootDistribution]
[Trade]
[Food]
[Drink]
[Potion]
[CriticalPotion]
[SunHelm]
[ActionQueue]
[Movement]
[Transition]
[Loading]
[Teleport]
[WakeUp]
[InventoryInteraction]
[CellChange]


Örnek:

[Transition]
Cell transition detected.
Follower AI suspended.

[Loading]
Cell loaded.

[Transition]
Stabilization started: 3 seconds.

[Transition]
Stabilization complete.
Follower AI resumed.

Inventory:

[InventoryInteraction]
Player opened follower inventory.
Autonomous actions suspended.

[InventoryInteraction]
Player closed follower inventory.
Resume delay: 2 seconds.

[InventoryInteraction]
Inventory re-evaluated.
Autonomous actions resumed.


============================================================
44. ÖNERİLEN STATE MACHINE
============================================================

READY
  ↓
NORMAL AI

LOADING
  ↓
STABILIZING
  ↓
READY

PLAYER INVENTORY
  ↓
WAITING
  ↓
RE-EVALUATE
  ↓
READY

TELEPORTING
  ↓
STABILIZING
  ↓
READY

WAKING
  ↓
STABILIZING
  ↓
READY

CRITICAL SURVIVAL
  ↓
ACTION
  ↓
READY

Not [YENİ]: CRITICAL SURVIVAL state'i, PLAYER INVENTORY / TELEPORTING / WAKING / STABILIZING state'lerinin üzerinden geçebilir, LOADING üzerinden geçemez. Aksiyon bittikten sonra önceki state'e geri dönülür.


============================================================
45. UYGULAMA ÖNCELİĞİ [GÜNCELLENDİ]
============================================================

FAZ 0 — TEŞHİS ALTYAPISI (İLK YAPILACAK) [YENİ]

0.1 Movement / SpeedMult log altyapısı
0.2 Aksiyon öncesi snapshot / sonrası restore mekanizması
0.3 Aksiyon geçmişi logu (son N aksiyon)
0.4 Merkezi debug log kategorileri


FAZ 1 — KRİTİK HATALAR

1. Classbound → King Core → Follower Unbound sınıf senkronizasyonu (boş/erişilemez durumlar dahil)
2. Yer ganimeti toplama
3. Protected Equipment (FormID + miktar mantığı dahil)
4. Reset Protected Equipment
5. Gereksiz eşya satışı
6. Food/Drink/Potion inventory synchronization
7. SunHelm drink desteği
8. Movement slowdown kök neden analizi ve düzeltmesi (Faz 0 loglarıyla)


FAZ 2 — LOOT VE ECONOMY

9. Round-robin loot distribution (kalıcı sıra sayacı)
10. Bulk loot
11. Smart selling
12. Smart shopping
13. Stock calculation
14. Food/Drink stock
15. Potion stock
16. Arrow/Bolt stock


FAZ 3 — ÇEKİRDEK: ACTION QUEUE + TRANSITION MANAGER (BİRLİKTE TASARLANIR) [GÜNCELLENDİ]

17. Merkezi Follower State / State Machine
18. Merkezi Action Queue (performans sınırlarıyla)
19. Transition Manager (Action Queue ile aynı anda, aynı arayüz üzerinden)
20. Drink → 3 saniye → Food
21. Aynı anda animation engelleme
22. Sequential follower processing
23. SunHelm animation fallback
24. Target completion
25. Sonsuz döngü ve zaman aşımı koruması


FAZ 4 — POTION

26. Class-independent potion system
27. Normal Health threshold
28. Normal Magicka threshold
29. Normal Stamina threshold
30. Critical Health
31. Critical Magicka
32. Critical Stamina
33. Emergency cooldown bypass
34. Critical potion + loading/inventory/teleport istisna kuralları


FAZ 5 — TRANSITION ÖZELLİKLERİ

35. Teleport stabilization
36. Wake-up stabilization
37. Player inventory lock
38. Cell transition detection
39. Loading protection
40. Post-load stabilization
41. Worldspace transition
42. Merchant transition protection
43. Ayarlanabilir gecikmeler + alt/üst sınırlar (clamp)


FAZ 6 — TEST

44. Tek takipçi
45. İki takipçi
46. Üç takipçi
47. Dört+ takipçi
48. Classbound Elementalist
49. Diğer Classbound sınıfları
50. King Core aktif/pasif/boş sonuç
51. SunHelm aktif
52. SunHelm pasif
53. Büyük loot testleri
54. Round-robin testleri (sıra devamı dahil)
55. Merchant sell/buy
56. Equipment protection (çoklu kopya dahil)
57. Equipment reset
58. Food/Drink queue
59. Animation fallback
60. Normal potion
61. Critical potion (istisna senaryoları dahil)
62. Arrow/Bolt
63. Movement stress + SpeedMult sapma testi
64. Teleport
65. Wake-up
66. Cell transition
67. Loading Screen
68. Player inventory interaction
69. Performans / kuyruk sınırı testi


============================================================
46. KABUL TESTLERİ
============================================================

TEST A — CLASSBOUND

Classbound:
Elementalist

King Core:
Elementalist

Follower Unbound:
Elementalist

Fallback Mage olsa bile final sonuç Elementalist olmalıdır.


TEST B — LOOT

3 follower:

1 Helmet
1 Gloves
1 Boots
3 Potions
2 Weapons

Sonuç:

Follower 1:
Helmet + Potion + Weapon

Follower 2:
Gloves + Potion + Weapon

Follower 3:
Boots + Potion


TEST C — EQUIPMENT PROTECTION

Follower:

Robe
Hood
Gloves
Boots

Reset Protected Equipment.

Merchant'a git.

Sonuç:

Protected equipment → Satılamaz
Enemy loot → Satılabilir


TEST D — SUNHELM

SunHelm aktif:

Drink shortage
    ↓
SunHelm-compatible drink detected
    ↓
Purchase
    ↓
Drink


TEST E — SEQUENTIAL NEEDS

3 follower:

Follower 1 → Drink → Food → Finish
Follower 2 → Drink → Food → Finish
Follower 3 → Drink → Food → Finish

Aynı anda iki follower yemek/içmek yapmamalıdır.


TEST F — CRITICAL POTION

Health = 10%
Critical threshold = 15%

Sonuç:

Immediate Health Potion

Normal cooldown beklenmemelidir.


TEST G — MOVEMENT

Follower:

Follow
Combat
Eat
Drink
Sleep
Wake
Teleport
Cell Transition
Trade
Loot

işlemlerinden sonra normal hızda hareket etmeye devam etmelidir.

Ek [YENİ]: Her aksiyon sonrası SpeedMult, aksiyon öncesi snapshot değerine eşit olmalıdır.


TEST H — TELEPORT

Teleport
    ↓
Stabilization
    ↓
No immediate autonomous action
    ↓
Ready
    ↓
Normal AI


TEST I — PLAYER INVENTORY

Player opens follower inventory
    ↓
Follower autonomous systems pause
    ↓
Player changes equipment/items
    ↓
Player closes inventory
    ↓
1–2 sec delay
    ↓
Inventory re-evaluation
    ↓
Normal AI


TEST J — CELL TRANSITION

Follower action active
    ↓
Enter cave/city/building
    ↓
AI suspended
    ↓
Loading
    ↓
Cell loaded
    ↓
2–3 sec stabilization
    ↓
AI resumed

Loading sırasında hiçbir normal autonomous action çalışmamalıdır.


TEST Q — KING CORE BOŞ SONUÇ [YENİ]

King Core yüklü, takipçi için sınıf döndürmüyor.

Sonuç:

Fallback sınıf geçici kullanılır.
Sonraki cell yüklemesinde King Core tekrar sorgulanır.
Crash yok, iksir/loot/ticaret sistemleri çalışmaya devam eder.


TEST R — ÇOKLU KOPYA KORUMASI [YENİ]

Takipçide 3 Iron Armor var, 1 tanesi giyili ve korumalı.

Merchant'a git.

Sonuç:

Giyili olan satılmaz.
Fazla 2 kopya satılabilir.
Şüpheli durumda koruma lehine karar verilir.


TEST S — ROUND-ROBIN SIRA DEVAMI [YENİ]

3 takipçi. Loot olayı 1: 2 item. Loot olayı 2: 2 item.

Sonuç:

Olay 1: Follower 1, Follower 2
Olay 2: Follower 3, Follower 1

Takipçi 2 kapasite dolu veya ölü ise atlanır.


TEST T — KRİTİK İKSİR İSTİSNALARI [YENİ]

Senaryo 1: Oyuncu takipçi envanterini açıkken Health = 10%.
Sonuç: Acil iksir kullanılır, diğer otomatik işlemler askıda kalır.

Senaryo 2: Loading sırasında Health = 10%.
Sonuç: Loading bitene kadar beklenir; loading bitince stabilizasyon atlanıp hemen iksir kullanılır.


TEST U — GECİKME SINIRLARI [YENİ]

MCM'de gecikme 0 sn ve 60 sn girilir.

Sonuç:

Değerler min/max sınırına çekilir ve loglanır.


TEST V — PERFORMANS [YENİ]

4+ takipçi, büyük loot, bulk trade, aynı anda ihtiyaç kuyruğu.

Sonuç:

Kuyruk uzunluğu sınırı aşılmaz.
Frame başına en fazla 1 ağır aksiyon çalışır.
Zaman aşımına uğrayan aksiyon state'i kilitlemez.


============================================================
47. PROTECTED EQUIPMENT SLOT KABUL TESTLERİ
============================================================

TEST K — TEMEL SLOT'LAR

Follower:

Head
Body
Hands
Feet

Reset Protected Equipment.

Sonuç:

30 → Protected
32 → Protected
33 → Protected
37 → Protected


TEST L — MODLU EKİPMAN

Follower:

Body
Cloak
Backpack
Belt
Pauldrons

Reset.

Sonuç:

32 → Protected
46 → Protected
47 → Protected
60 → Protected
57 → Protected


TEST M — AKSESUARLAR

Follower:

Circlet
Amulet
Ring
Ears
Mask

Reset.

Sonuç:

42 → Protected
55 → Protected
56 → Protected
43 → Protected
44 → Protected


TEST N — SATIŞ

Protected Robe → SELL BLOCKED

Enemy Robe → SELL ALLOWED


TEST O — EQUIPMENT DEĞİŞTİRME

Protected:
Iron Armor

Oyuncu:
Steel Armor giydiriyor.

Reset yapılmadıysa:

Iron Armor → Protected
Steel Armor → Normal

Reset yapılırsa:

Steel Armor → Protected


TEST P — ÇOKLU TAKİPÇİ

Follower A → Robe protected
Follower B → Armor protected
Follower C → Cloak protected

Follower B resetlenir.

Sonuç:

Follower A → Değişmez
Follower B → Yeni koruma kaydı
Follower C → Değişmez


============================================================
48. NİHAİ MİMARİ
============================================================

                    +------------------+
                    |    Classbound    |
                    +--------+---------+
                             |
                             v
                    +------------------+
                    |    King Core     |
                    +--------+---------+
                             |
                             v
                    +------------------+
                    | Follower Unbound |
                    +--------+---------+
                             |
                             v
                  +-----------------------+
                  |    Follower State     |
                  |     / State Machine   |
                  +-----------+-----------+
                              |
                              v
                  +-----------------------+
                  |  Transition Manager  |
                  +-----------+-----------+
                              |
                              v
                  +-----------------------+
                  |     Action Queue     |
                  |  (limitler + timeout) |
                  +-----------+-----------+
                              |
        +----------+----------+----------+-----------+
        |          |          |          |           |
        v          v          v          v           v
      Needs      Loot      Trading    Potions    Equipment
        |          |          |          |           |
      Food      Loot      Buy/Sell  Emergency   Protection
      Drink     Share      Stock     Normal       Reset


Protected Equipment:

Follower
   ↓
Equipment Scanner
   ↓
Supported Slot List
   ↓
Current Equipped Item
   ↓
Protected Equipment Registry (FormID + miktar + varsa instance verisi)
   ↓
Trade/Sell Protection
Inventory Cleanup Protection
Equipment Integration


============================================================
49. EN ÖNEMLİ KURALLAR [GÜNCELLENDİ]
============================================================

Follower Unbound normal autonomous action'ları şu durumlarda çalıştırmamalıdır:

- Loading Screen
- Cell Transition
- Worldspace Transition
- Teleport Stabilization
- Wake-Up Stabilization
- Player Inventory Interaction

PLAYER CONTROL her zaman FOLLOWER AUTONOMOUS ACTIONS üzerinde önceliğe sahip olmalıdır.

Tek istisna CRITICAL SURVIVAL'dır:
- Oyuncu envanteri açıkken, teleport ve wake-up stabilizasyonunda çalışabilir.
- Loading/cell transition sırasında ertelenir; loading bitince stabilizasyon atlanarak hemen çalışır.

Protected Equipment sistemi:

"Bu slotu asla değiştirme"

anlamına gelmez.

Doğru anlam:

"Bu anda bu slotta bulunan bu belirli ekipmanı otomatik satıştan ve inventory temizliğinden koru."

Şüpheli durumlarda (belirsiz item kimliği) koruma lehine karar verilir.

Reset Protected Equipment oyuncunun yeni ekipman setini korumaya almasını sağlar.

Genel loot sistemi Protected Equipment nedeniyle kapatılmamalıdır.

Classbound entegrasyonu doğrudan değil:

Classbound → King Core → Follower Unbound

şeklinde olmalıdır. King Core boş veya erişilemezse fallback devreye girer, ancak King Core geçerli sonuç verdiğinde fallback onu asla geçersiz kılamaz.

Transition ve Action Queue sistemleri mümkün olduğunca merkezi olmalı, her özellik için bağımsız timer sistemleri oluşturulmamalıdır.

Hız (SpeedMult) değişiklikleri her zaman snapshot/restore ile korunmalıdır.

Kullanıcı tanımlı gecikmeler her zaman min/max sınırları içinde tutulmalıdır.


============================================================
50. NİHAİ HEDEF
============================================================

Follower Unbound yalnızca "takipçiyi otomatik hareket ettiren" bir mod olmamalıdır.

Takipçinin:

- Sınıf bilgisini
- Envanterini
- İhtiyaçlarını
- Yiyeceklerini
- İçeceklerini
- İksirlerini
- Mühimmatını
- Ganimetlerini
- Ticaretini
- Ekipmanlarını
- Korunan kıyafetlerini
- Hareket durumunu
- Teleport durumunu
- Wake-up durumunu
- Cell/worldspace transition durumunu
- Oyuncu kontrol durumunu

birbirleriyle çakışmadan yöneten merkezi, güvenilir ve genişletilebilir bir AI sistemi oluşturulmalıdır.

Öncelik:

GÜVENİLİRLİK
    ↓
OYUNCU KONTROLÜ
    ↓
STATE STABİLİTESİ
    ↓
CRITICAL SURVIVAL
    ↓
NEEDS
    ↓
INVENTORY / TRADE
    ↓
LOOT
    ↓
NORMAL AI

Bu sistem Follower Unbound'ın mevcut mimarisine mümkün olduğunca az gereksiz bağımlılık eklemeli ve King Core entegrasyonunu korumalıdır.
