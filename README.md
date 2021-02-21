# Zmodyfikowany Problem Placzy Tytoniu

Konrad Szychowiak 144564

## Kompilacja

```shell
gcc main.c sem.h wait.h
```

Tak skompilowany plik powinien działać. Występowały problemy z zarządzaniem pamiecią przy kompilacji z użyciem `clang`.

## Zadanie

* 3 **palaczy** siedzi przy stole.
* Do zapalenia papierosa potrzebują 3 składników (tu: tytoń, zapałki, papier).
* Każdy palacz dysponuje nieskończonym zapasem **jednego** składnika oraz portwelem z (początkowo) niezerową liczba
  pieniędzy.
* Pozostałe składniki muszą zostać kupione od pozostałych palaczy, po ustalonej cenie rynkowej.
* Cenę rynkową ustala osobno działający **agent**.
* Palacz może kupić tylko oba składniki na raz (jeśli go stać), lub nic.
* Jeżeli palacz kupił potrzebne produkty to skręca i zapala papierosa.

## Koncepcja rozwiązania

* Palacze stanowią 3 niezależne procesy. Dzielą oni kod, ponieważ różnice w ich działaniu sprowadzają się jedynie do
  produktów produkowanych
  (co również jednoznacznie dla każdego identyfikuje pozostałe potrzebne produkty);
* Proces-rodzic uruchamia procesy palaczy, a następnie realizuje funkcjonalność agenta;
* Każdy palacz ma swoją kolejkę komunikatów, przez którą odbiera kupione produkty i opłaty na poczet wysłania produktó
  przez siebie produkowanego;
* Zasobem krytycznym są cany przechowywane w segmencie pamieci współdzielonej. Dostęp do nich kontrolowany jest przez
  pojedynczy semafor. Ceny używane są:

    - przez **palaczy**: do odjęcia ze swojego portfela kosztu kupowanych składników i przesłania zapłaty pozostałym
      palaczom poprzez kolejke komunikatów,
    - przez **agenta**: zmienia on ceny rynkowej.

  Ceny nie mogą ulec zmianie podczas przekazywania opłat albo modyfikacji własnego portfela. Dodawanie do własnego
  portfela nie wymaga sekcji krytycznej, ponieważ mamy gwarancję, że przychodząca kwota była zgodna z ceną rynkową w
  momencie wysłania
  (który to moment nie jest dokładnie znany, ponieważ wyszystkie operacje są wykonywane asynchronicznie).

* Po opuszczeniu sekcji krytycznej każdy palacz odbiera wiadomości ze swojej kolejki w dwóch turach:
    1. czyta _**nie**blokująco_ wszystkie dostępne aktualnie wiadomości,
    1. czyta _blokująco_ wiadomości aż do odebrania zakupionych produktów
       (zatem ten ktok się nie wykona, jezeli nie kupił żadnych produktów, lub odebrał oba w poprzednik kroku).
       
### Użyte obiekty IPC System V

* 1 segment pamięci współdzielonej z cenami produktów (trzyma tablicę 4 elementów, ponieważ identyfikatory produktów są z zakresu [1;3],
  a łatwiej i bezpieczniej było dodać trochę wiecej pamięci niż wykonywać nieustannie dodawanie i odejmowanie elemntów)
* 1 pojedynczy semafor używany w kontroli dostepu do cenn (opisane wyżej)
* 3 kolejki komunikatów przypisane do palaczy