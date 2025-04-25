from flask import Flask, render_template, request, redirect, url_for, flash, session
from des import encrypt, decrypt
import sqlite3
import hashlib
import random

app = Flask(__name__)
app.secret_key = 'super_secret_key'  # 用于 session 和 flash 消息

def hash_sha256(s):
    return hashlib.sha256(s.encode()).hexdigest()

@app.route('/login', methods=['GET', 'POST'])
def login():
    if request.method == 'POST':
        username = request.form['username']
        password = request.form['password']
        hashed1 = hash_sha256(username + password)
        nonce = ''.join(random.choices('0123456789', k=8))

        conn = sqlite3.connect('users.db')
        c = conn.cursor()
        c.execute('SELECT hashed FROM users WHERE username=?', (username,))
        result = c.fetchone()
        conn.close()

        if not result:
            flash("用户不存在，请先注册", "error")
            return render_template('login.html')

        stored_hashed = result[0]
        if stored_hashed != hashed1:
            flash("密码错误，请重试", "error")
            return render_template('login.html')

        encrypted_nonce = encrypt(nonce, key=hashed1[:8])  # 使用前8位作为DES key
        decrypted_nonce = decrypt(encrypted_nonce, key=hashed1[:8])

        # 登录成功写入日志
        with open('login_log.txt', 'a') as f:
            from datetime import datetime
            f.write(f'{datetime.now()} - {username} 登录成功，认证码：{decrypted_nonce}\n')

        flash("登录成功！", "success")
        return redirect(url_for('login'))

    return render_template('login.html')


@app.route('/register', methods=['GET', 'POST'])
def register():
    if request.method == 'POST':
        username = request.form['username']
        password = request.form['password']
        hashed1 = hash_sha256(username + password)

        # 检查用户是否已存在
        conn = sqlite3.connect('users.db')
        c = conn.cursor()
        c.execute('SELECT username FROM users WHERE username=?', (username,))
        result = c.fetchone()

        if result:
            flash("用户名已存在，请选择其他用户名", "error")
            return render_template('register.html')

        # 将用户信息存储到数据库
        c.execute('INSERT INTO users (username, hashed) VALUES (?, ?)', (username, hashed1))
        conn.commit()
        conn.close()

        flash("注册成功，请登录", "success")
        return redirect(url_for('login'))

    return render_template('register.html')

@app.route('/')
def index():
    return redirect(url_for('login'))



if __name__ == '__main__':
    app.run(debug=True)
