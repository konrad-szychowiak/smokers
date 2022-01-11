# Zmodyfikowany Problem Placzy Tytoniu

Konrad Szychowiak

## Kompilacja

```shell
gcc main.c sem.h wait.h
```

Program kompilowany gcc działa bez problemu. Występowały jednak magiczne problemy z zarządzaniem pamiecią przy kompilacji z użyciem kompilatora `clang`.

## Zadanie

* 3 **palaczy** siedzi przy stole.
* Do zapalenia papierosa potrzebują 3 składników (tu: tytoń, zapałki, papier).
* Każdy palacz dysponuje nieskończonym zapasem **jednego** składnika oraz portfelem z (początkowo) niezerową ilością
  pieniędzy.
* Pozostałe składniki muszą zostać kupione od pozostałych palaczy, po ustalonej cenie rynkowej.
* Aktualną cenę rynkową ustala w losowych kwantach czasu osobno działający **agent**.
* Palacz może kupić tylko oba składniki na raz (jeśli go stać), lub nic.
* Jeżeli palacz kupił potrzebne produkty to skręca i zapala papierosa.

## Koncepcja rozwiązania

* Palacze stanowią 3 niezależne procesy. Dzielą oni kod, ponieważ różnice w ich działaniu sprowadzają się jedynie do
   produkowanych sładników (co również jednoznacznie dla każdego identyfikuje pozostałe potrzebne produkty);
* Proces-rodzic uruchamia procesy palaczy, a następnie realizuje funkcjonalność agenta;
* Każdy palacz ma swoją kolejkę komunikatów, przez którą odbiera kupione produkty i opłaty na poczet wysłania pozostałym produktu
  przez siebie produkowanego;
* Zasobem krytycznym są ceny przechowywane w segmencie pamieci współdzielonej. Dostęp do nich kontrolowany jest przez
  pojedynczy semafor. Wspóldzielony obszar cen używany jest:

    - przez **palaczy**: do odjęcia (loklanie) ze swojego portfela kosztu kupowanych składników i przesłania zapłaty pozostałym
      palaczom poprzez kolejke komunikatów,
    - przez **agenta**: zmienia on ceny rynkowe.

  Ceny nie mogą ulec zmianie podczas przekazywania opłat albo modyfikacji własnego portfela (sekcja krytyczna). Dodawanie do własnego
  portfela odebranej z kolejki zapłaty nie wymaga sekcji krytycznej, ponieważ mamy gwarancję, że przychodząca kwota była zgodna z ceną rynkową w
  momencie wysłania
  (który to moment nie musi być dokładnie znany, gdyż wyszystkie operacje są wykonywane asynchronicznie).

* Po opuszczeniu sekcji krytycznej każdy palacz odbiera wiadomości ze swojej kolejki w dwóch turach:
    1. czyta _**nie**blokująco_ wszystkie dostępne już wiadomości,
    1. czyta _blokująco_ wiadomości aż do odebrania oczekiwanej liczby zakupionych produktów
       (zatem ten ktok się nie wykona, jeżeli palacz nie kupił żadnych produktów, lub odebrał oba w poprzednim kroku).
       
### Użyte obiekty IPC System V

* 1 segment pamięci współdzielonej z cenami produktów (trzyma tablicę 4 elementów, ponieważ identyfikatory produktów są z zakresu [1;3],
  a łatwiej i bezpieczniej było dodać trochę wiecej pamięci niż wykonywać nieustannie dodawanie i odejmowanie indeksów elementów)
* 1 pojedynczy semafor używany w kontroli dostepu do cen (opisane wyżej)
* 3 kolejki komunikatów przypisane do palaczy (każdy ma swoją kolejkę "odbiorczą")

Program działa aż do momentu jawnego przerwania. Usuwanie obiektów IPC dokonywane jest ręcznie 
(oczywiście można usuwać je w obsłudze sygnału SIGINT/SIGTERM).
