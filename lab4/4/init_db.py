import sqlite3

def init_db():
    # 连接数据库
    conn = sqlite3.connect('users.db')
    c = conn.cursor()

    # 创建用户表，如果表已存在不会重复创建
    c.execute('''
    CREATE TABLE IF NOT EXISTS users (
        username TEXT PRIMARY KEY,
        hashed TEXT
    )
    ''')

    # 提交并关闭连接
    conn.commit()
    conn.close()

if __name__ == '__main__':
    init_db()
