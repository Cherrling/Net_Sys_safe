import hashlib
import json
import random
import requests
from des import encrypt, decrypt
from datetime import datetime

SERVER_URL = "http://127.0.0.1:5000"

def generate_nonce():
    return ''.join(str(random.randint(0, 9)) for _ in range(8))

def hash_sha256(data):
    return hashlib.sha256(data.encode()).hexdigest()

def register(username, password):
    hash1 = hash_sha256(username + password)
    response = requests.post(f"{SERVER_URL}/register", json={"username": username, "hash1": hash1})
    print(response.text)

def login(username, password):
    hash1 = hash_sha256(username + password)
    nonce = generate_nonce()
    hash2 = hash_sha256(hash1 + nonce)

    payload = {
        "username": username,
        "nonce": nonce,
        "hash2": hash2
    }

    response = requests.post(f"{SERVER_URL}/login", json=payload)

    if response.status_code == 200:
        data = response.json()
        encrypted_nonce = data.get("encrypted_nonce")
        decrypted_nonce = decrypt(encrypted_nonce, hash1[:8])

        print(f"[客户端] 解密后的认证码: {decrypted_nonce}")

        if decrypted_nonce == nonce:
            print("✅ 登录成功，认证码匹配")
            with open("login_log.txt", "a", encoding="utf-8") as f:
                f.write(f"登录成功：用户名={username}, 时间={datetime.now()}, 认证码={decrypted_nonce}\n")
        else:
            print("❌ 登录失败，认证码不匹配")
    else:
        print("❌ 登录失败", response.text)

if __name__ == "__main__":
    print("== 客户端模拟 ==")
    choice = input("1: 注册 | 2: 登录 >> ")
    username = input("请输入用户名: ")
    password = input("请输入密码: ")

    if choice == "1":
        register(username, password)
    elif choice == "2":
        login(username, password)
