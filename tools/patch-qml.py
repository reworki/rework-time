"""Правки Main.qml: иконки реально показывать, номер места — внизу колонки."""
import io
import re

path = r'C:\KNOT\REWORK-TIME\qml\Main.qml'
src = io.open(path, encoding='utf-8').read()

# 1. Image: source — это url, у него нет .length, поэтому картинки были скрыты
before = src
src = src.replace('visible: source.length > 0', 'visible: ("" + source).length > 0')
print('image visible fixed:', src != before)

# 2. Номер места — у нижнего края колонки, чтобы не спорить с иконкой
rank_old = re.compile(
    r'Text \{\s*\n\s*anchors\.(?:centerIn|horizontalCenter): parent[^\n]*\n'
    r'(?:[^\n]*\n){0,3}?\s*text: barItem\.rank \+ 1',
    re.M)
candidate = re.search(r'text: barItem\.rank \+ 1', src)
print('rank text found:', bool(candidate))

block_start = src.rfind('Text {', 0, candidate.start())
block_end = src.find('}', candidate.end())
print('block span:', block_start, block_end)

replacement = '''Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: 8
                            text: barItem.rank + 1
                            color: "#080b12"
                            opacity: 0.3
                            font.family: "Consolas"
                            font.pixelSize: 52
                            font.bold: true
                        }'''
if block_start != -1 and block_end != -1:
    src = src[:block_start] + replacement + src[block_end + 1:]

io.open(path, 'w', encoding='utf-8', newline='').write(src)
print('ok', len(src))
