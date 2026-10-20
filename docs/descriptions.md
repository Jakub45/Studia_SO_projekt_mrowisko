**Temat 2 – Mrowisko.**

Początkowy stan symulacji zakłada istnienie populacji złożonej z M aktywnych mrówek robotnic. W 

głębi gniazda znajduje się centralny spichlerz, w którym jednocześnie przebywać może co najwyżej 

K osobników (przy zastrzeżeniu warunku K < M/2.). 



Napisz programy symulujące działanie mrowiska: królowa, mrówka, przyroda.

Raport z przebiegu symulacji (logi) zapisać w pliku (plikach) tekstowym.



**Dostęp z powierzchni do centralnego spichlerza**

odbywa się poprzez system trzech dwukierunkowych, lecz wąskich szybów transportowych. Szyby te 

stanowią sekcje krytyczne o zmiennej lokalnie przepustowości: w danym punkcie czasowym 

konkretny szyb umożliwia ruch wyłącznie w jednym kierunku (z powierzchni do wnętrza lub 

odwrotnie), co wymaga zaimplementowania odpowiedniego algorytmu arbitrażu i zapobiegania 

zakleszczeniom. Jeżeli proces czeka na wejście do mrowiska dłużej niż Tc dokonuje kontrolowanego 

zakończenia działania i zwolnienia zasobów systemowych.



**Każda mrówka robotnica realizuje powtarzalny cykl aprowizacyjny**. 

Po przejściu przez szyb transportowy robotnica spędza w spichlerzu losowy czas Di, przeznaczony na wyładunek zasobów,

po czym opuszcza wnętrze gniazda. Długość życia każdej robotnicy jest ściśle ograniczona i 

zdefiniowana przez maksymalną liczbę Ei ukończonych cykli transportowych. Po przekroczeniu 

progu Ei proces reprezentujący daną robotnicę dokonuje kontrolowanego zakończenia działania i 

zwolnienia zasobów systemowych.



**Za reprodukcję populacji odpowiada osobny proces reprezentujący królową**. 

W zadanych odstępachczasu Tink królowa inicjuje proces inkubacji nowego pokolenia. Złożenie i wylęg nowych larw 

(przekształcanych bezpośrednio w nowe procesy robotnic) jest możliwe wyłącznie pod warunkiem 

dostępności wolnej objętości w komorze lęgowej, wyliczanej na podstawie bieżącego całkowitego 

limitu pojemności gniazda. Gdy zabraknie królowej, populacja mrowiska stopniowo zanika.



**Zewnętrzna ingerencja w środowisko symulacji realizowana jest za pomocą asynchronicznej obsługi** 

**sygnałów przesyłanych do procesu głównego:** 



− **Odbiór pierwszego sygnału dedykowanego (SIGUSR1)** symuluje rozbudowę mrowiska –

powoduje dynamiczne zwiększenie maksymalnej dopuszczalnej pojemności gniazda do poziomu 

nieprzekraczającego 2.0 ⋅ K . Nie ma możliwości powiększenia populacji ponad podane 

maksimum.

− **Odbiór drugiego sygnału dedykowanego (SIGUSR2)** reprezentuje czynniki destrukcyjne (np. 

osunięcie ziemi) – skutkuje natychmiastową redukcją bieżącej pojemności operacyjnej mrowiska 

o 50%, co wymaga bezzwłocznej ewakuacji nadmiarowych osobników ze spichlerza oraz 

wstrzymania nowych wejść. Populacja może być redukowana wielokrotnie, aż do liczby K=1

− **Odbiór trzeciego sygnału powoduje zamknięcie jednego z trzech wejść do mrowiska.** Numer 

wejścia, które ma być zamknięte, jest podany jako argument funkcji sigaction(). Możliwe jest 

zamknięcie wszystkich wejść. Po upłynięciu czasu Tk po zamknięciu ostatniego wejścia 

wszystkie mrówki znajdujące się w mrowisku wraz z królową dokonują kontrolowanego 

zakończenia działania i zwolnienia zasobów systemowych.





