c = 'a'

lines = [
    'BLABLABLALA',
    'TRUSE',
    'PROMP',
    'TOBIAS E KUL',
    'NINJAGO',
    'MAMMA SITTER I DO',
    'PROMPELINESKUM',
    'BLIBLABLELBULEUDLADLADLADALDALLADDBLIBLEIDEBIB ALLAB ALLABA BLAL BAB LAB LAB A',
    'da va ikke ferdi'
]

from random import randint

while c != 'q':
    c = input("SKRIV ET TALL: ")
    idx = randint(0, len(lines) - 1)
    print(lines[idx])
