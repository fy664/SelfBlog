# -*- coding: utf-8 -*-
"""
SelfBlog 增量部署脚本（本机 Windows 运行）
==========================================
用法：
  set BLOG_SERVER=root@your-server-ip     # 先设置服务器地址（不含 IP，避免泄露）
  python deploy_push.py            # 全量：前端 + 后端 + 数据库迁移
  python deploy_push.py --fe       # 只推前端（秒级生效，无需重启）
  python deploy_push.py --be       # 只推后端源码并在服务器编译重启
  python deploy_push.py --db       # 只跑数据库幂等迁移

原理：
  - 前端是静态文件：打包 web/dist -> scp 上传 -> 服务器解包到 nginx 目录，
    上传完成即生效，nginx 不用重启。
  - 后端是 C++ 编译型：推送源码 -> 服务器上编译 -> 平滑重启（杀旧进程起新进程，
    中断 1-2 秒）。
  - 数据库：schema.sql 幂等迁移，随时可跑。
"""
import os
import subprocess
import sys

# 服务器地址从环境变量读取（git 仓库不含真实 IP）
SERVER = os.environ.get("BLOG_SERVER") or "root@your-server-ip"
if "your-server-ip" in SERVER:
    print("!! 请先设置服务器地址：set BLOG_SERVER=root@<服务器IP>")
    sys.exit(1)
ROOT = os.path.dirname(os.path.abspath(__file__))
SHOTS = os.path.join(ROOT, "_shots")
os.makedirs(SHOTS, exist_ok=True)


def run(cmd, desc, check=True):
    print("\n>> " + desc)
    r = subprocess.run(cmd, capture_output=True, text=True,
                       encoding="utf-8", errors="replace")
    if r.stdout.strip():
        print(r.stdout.strip()[:600])
    if r.returncode != 0:
        print("!! FAILED:")
        print(r.stderr.strip()[-400:])
        if check:
            sys.exit(1)
    return r


def ssh(cmd, desc, check=True):
    return run(["ssh", "-o", "BatchMode=yes", "-o", "ConnectTimeout=10",
                SERVER, cmd], desc, check)


def push_frontend():
    tgz = os.path.join(SHOTS, "push_fe.tgz")
    if os.path.exists(tgz):
        os.remove(tgz)
    run(["tar", "-cf", tgz, "-C", os.path.join(ROOT, "web", "dist"), "."],
        "打包前端 dist")
    run(["scp", "-q", tgz, SERVER + ":/tmp/push_fe.tgz"], "上传前端包到服务器")
    ssh("cd /opt/selfblog/web && rm -rf assets && tar -xf /tmp/push_fe.tgz "
        "&& rm /tmp/push_fe.tgz && ls index.html", "解包到 nginx 目录（秒级生效）")
    print("  -> 前端已更新")


def push_backend():
    tgz = os.path.join(SHOTS, "push_be.tgz")
    if os.path.exists(tgz):
        os.remove(tgz)
    # 打包源码 + CMake + 第三方头文件（config.json 不打包，服务器保留自己的）
    run(["tar", "-cf", tgz, "-C", os.path.join(ROOT, "server"),
         "src", "CMakeLists.txt", "third_party"], "打包后端源码")
    run(["scp", "-q", tgz, SERVER + ":/tmp/push_be.tgz"], "上传后端源码包")
    build = ("cd /opt/selfblog/server && "
             "tar -xf /tmp/push_be.tgz && rm /tmp/push_be.tgz && "
             "cmake -B build -DCMAKE_BUILD_TYPE=Release > /dev/null && "
             "cmake --build build -j2 && "
             "pkill -x blog_server || true; sleep 1; "
             "nohup ./build/blog_server > server.log 2>&1 & "
             "sleep 2; "
             "curl -s http://127.0.0.1:8080/api/health")
    ssh(build, "服务器解包源码 + 编译 + 重启后端（约 1-2 分钟）")
    print("  -> 后端已更新")


def push_db():
    run(["scp", "-q", os.path.join(ROOT, "sql", "schema.sql"),
         SERVER + ":/tmp/schema.sql"], "上传 schema.sql")
    ssh("mysql < /tmp/schema.sql && rm /tmp/schema.sql", "执行幂等数据库迁移")
    print("  -> 数据库已迁移")


def verify():
    base = "http://" + SERVER.split("@")[-1] + ":8081"
    run(["curl.exe", "-s", "-o", "NUL", "-w",
         "front: HTTP %{http_code}\nhealth: ",
         "--max-time", "10", base + "/"], "验证公网前端")
    run(["curl.exe", "-s", "-o", "NUL", "-w",
         "HTTP %{http_code}\n", "--max-time", "10",
         base + "/api/health"], "验证公网 API")


if __name__ == "__main__":
    args = set(sys.argv[1:])
    if not (args & {"--fe", "--be", "--db"}):
        args = {"--fe", "--be", "--db"}
    if "--fe" in args:
        push_frontend()
    if "--be" in args:
        push_backend()
    if "--db" in args:
        push_db()
    verify()
    print("\n完成。访问 http://%s:8081" % SERVER.split("@")[-1])
