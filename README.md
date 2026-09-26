# MySynth

MySynth — полифонический VST3-синтезатор для Windows и FL Studio.

## Возможности

- Два осциллятора.
- Формы волн: Sine, Saw, Square и Triangle.
- Регуляторы громкости и расстройки осцилляторов.
- ADSR-конверт: Attack, Decay, Sustain и Release.
- Low-pass-фильтр с регуляторами Cutoff и Resonance.
- Управление нотами через MIDI.
- Сохранение параметров в проекте DAW.

## Установка из ZIP

1. Скачайте ZIP-архив MySynth из раздела Releases.
2. Распакуйте архив.
3. Скопируйте папку `MySynth` целиком в:

   `C:\Program Files\Common Files\VST3\`

   
4. Перезапустите FL Studio.
5. Откройте __Options > Manage plugins__ и нажмите __Find installed plugins__.
6. Найдите `MySynth` среди генераторов и добавьте его в Channel Rack.

Для установки в системную папку Windows может запросить права администратора.

## Сборка из исходников

### Требования

- Windows 10 или новее, 64-разрядная версия.
- Visual Studio с компонентом «Разработка классических приложений на C++».
- JUCE и Projucer.

### Сборка

Проект `MySynth.jucer` ссылается на JUCE по относительному пути `../JUCE`. Для сборки разместите папку JUCE рядом с папкой проекта MySynth либо настройте пути модулей в Projucer.

1. Откройте `MySynth.jucer` в Projucer.
2. Сохраните проект и откройте его в Visual Studio.
3. Выберите конфигурацию `Release` и платформу `x64`.
4. Выполните __Сборка > Собрать решение__.

Готовый пакет VST3 появится по пути:

`Builds/VisualStudio2026/x64/Release/VST3/MySynth.vst3`

Чтобы подготовить ZIP для распространения, архивируйте весь пакет `MySynth.vst3`, а не только вложенный файл `.vst3`.

## Структура проекта

- `Source/PluginProcessor.h` и `Source/PluginProcessor.cpp` — обработка MIDI и генерация звука.
- `Source/PluginEditor.h` и `Source/PluginEditor.cpp` — графический интерфейс.
- `MySynth.jucer` — настройки проекта Projucer.
- `Builds/VisualStudio2026/` — проект Visual Studio.

## Лицензии

Исходный код MySynth распространяется на условиях лицензии, указанной в файле `LICENSE`, если такой файл включён в релиз. JUCE распространяется по собственной лицензии.