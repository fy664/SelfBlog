# -*- coding: utf-8 -*-
"""获取清华镜像 MySQL-8.0 目录列表，找最新 winx64 zip 的下载地址。"""
import re
import urllib.request

BASE = "https://mirrors.tuna.tsinghua.edu.cn/mysql/downloads/MySQL-8.0/"
req = urllib.request.Request(BASE, headers={"User-Agent": "Mozilla/5.0"})
with urllib.request.urlopen(req, timeout=30) as r:
    html = r.read().decode("utf-8", errors="replace")

# 提取形如 mysql-8.0.xx-winx64.zip 的文件名
names = re.findall(r"mysql-8\.0\.\d+-winx64\.zip", html)
print("找到候选:", names)
if names:
    print("URL:", BASE + names[-1])
