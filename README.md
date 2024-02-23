<h1>Финальный проект: поисковый сервер</h1>

  <h2>Описание проекта.</h2>
  Этот проект представляет собой программу на языке C++, которая добавляет данные в базу и находит самые релевантные из них в ответ на пользовательские запросы. При запуске программы создается объект базы данных, который принимает в конструкторе строку со стоп-словами. Данные, содержащие эти стоп-слова, не учитываются при поиске релевантности. Далее передаются сами данные в виде массива строк, которые корректно парсятся перед добавлением их в базу. После этого программа обрабатывает запрос в виде строки, находит самые релевантные данные в базе и выводит параметры этих данных в консоль в порядке убывания их релевантности.
  
  <h2>Как использовать.</h2><br>
  <b>Запуск программы:</b><br>
  1. Убедитесь, что на вашем компьютере установлен компилятор C++.<br>
  2. Клонируйте репозиторий: git clone https://github.com/Niazhub/cpp-search-server.git<br>
  3. Перейдите в папку проекта: cd репозиторий<br>
  4. Соберите проект: g++ main.cpp -o search_app -std=c++11<br>
  5. Запустите программу: ./search_app<br>
  <b>Настройка базы данных:</b><br>
  При создании объекта базы передайте строку стоп-слов в конструктор.<br>
  <b>Добавление данных:</b><br>
  Передайте данные в виде массива строк, они будут корректно распарсены и добавлены в базу.<br>
  <b>Выполнение запросов:</b><br>
  Введите запрос в виде строки. Программа найдет самые релевантные данные в базе и выведет их параметры в консоль.<br>
  <h2>Технологии и особенности.</h2>
  • Проект написан на языке C++ с использованием стандартных библиотек.<br>
  • Изучен базовый синтаксис C++, применены алгоритмы из STL.<br>
  • Использована многопоточность для ускорения работы алгоритмов, с устранением состояний гонки через мьютексы.<br>
  • Написаны собственные UNIT тесты для проверки корректности работы программы.<br>
  • Проект позволяет улучшить понимание макросов в C++.<br>

  
