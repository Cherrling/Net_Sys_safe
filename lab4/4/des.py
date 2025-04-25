import hashlib
from Crypto.Cipher import DES
import base64

def pad(text):
    while len(text) % 8 != 0:
        text += ' '
    return text

def get_hash1(username, password):
    return hashlib.sha256((username + password).encode()).hexdigest()

def get_hash2(hash1, nonce):
    return hashlib.sha256((hash1 + nonce).encode()).hexdigest()

def encrypt(text, key):
    key = pad(key[:8])
    des = DES.new(key.encode(), DES.MODE_ECB)
    text = pad(text)
    encrypted = des.encrypt(text.encode())
    return base64.b64encode(encrypted).decode()

def decrypt(cipher, key):
    key = pad(key[:8])
    des = DES.new(key.encode(), DES.MODE_ECB)
    decrypted = des.decrypt(base64.b64decode(cipher))
    return decrypted.decode().strip()
