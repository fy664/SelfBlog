# -*- coding: utf-8 -*-
"""探测多个 MySQL 8 winx64 zip 镜像源的可用性。"""
import urllib.request

UA = {"User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64)"}
CANDIDATES = [
    ("阿里云", "https://mirrors.aliyun.com/mysql/Downloads/MySQL-8.0/mysql-8.0.42-winx64.zip"),
    ("网易", "https://mirrors.163.com/mysql/Downloads/MySQL-8.0/mysql-8.0.42-winx64.zip"),
    ("腾讯", "https://mirrors.cloud.tencent.com/mysql/Downloads/MySQL-8.0/mysql-8.0.42-winx64.zip"),
    ("华为", "https://mirrors.huaweicloud.com/mysql/Downloads/MySQL-8.0/mysql-8.0.42-winx64.zip"),
    ("官方CDN", "https://cdn.mysql.com/Downloads/MySQL-8.0/mysql-8.0.42-winx64.zip"),
    ("清华", "https://mirrors.tuna.tsinghua.edu.cn/mysql/downloads/MySQL-8.0/mysql-8.0.42-winx64.zip"),
]

for name, url in CANDIDATES:
    req = urllib.request.Request(url, headers=UA, method="HEAD")
    try:
        with urllib.request.urlopen(req, timeout=20) as r:
            length = r.headers.get("Content-Length")
            print(f"[OK] {name}: {url}  size={length}")
    except Exception as e:
        print(f"[失败] {name}: {type(e).__name__} {e}")
