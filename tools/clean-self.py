"""Убрать из базы записи о самом Rework Time и показать топ."""
import os
import sqlite3
import sys

sys.stdout.reconfigure(encoding='utf-8')

db = os.path.join(os.environ['APPDATA'], 'REWORK', 'ReworkTime', 'usage.db')
con = sqlite3.connect(db)
removed = con.execute("DELETE FROM sessions WHERE app = 'reworktime'").rowcount
con.commit()
print('удалено строк:', removed)
for app, spent in con.execute(
        'SELECT app, SUM(end_ts - start_ts) AS s FROM sessions GROUP BY app ORDER BY s DESC LIMIT 5'):
    print(f'  {app}: {spent} сек')
