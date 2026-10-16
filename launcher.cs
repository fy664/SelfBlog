// ============================================================
// 全栈个人博客系统项目书 · 启动器（C# / .NET Framework 4.x）
// 作用：把内嵌的 HTML 项目书释放到系统临时目录，并用系统默认浏览器打开，
// 得到"双击即用"的免安装 exe；页面保持纯 HTML，可离线阅读、打印、导出 PDF。
// 编译命令（Win10/11 自带 .NET Framework，csc 位于 C:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe）：
//   csc /nologo /target:winexe /r:System.Windows.Forms.dll \
//       /resource:全栈个人博客系统项目书.html,pb.html \
//       /out:全栈个人博客系统项目书.exe launcher.cs
// 语法要点：/resource:<文件>,<资源名> 把 HTML 作为内嵌资源打进 exe；
//          资源名 pb.html 与下方 GetManifestResourceStream("pb.html") 必须一致。
// ============================================================
using System;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Windows.Forms;

// 静态类 Program：C# 控制台/WinForms 程序的标准入口所在
internal static class Program
{
    // 入口方法：[STAThread] 为 MessageBox 等 WinForms 控件必需的线程模型；
    // /target:winexe 下无控制台窗口，正常流程不弹窗
    [STAThread]
    private static void Main(string[] args)
    {
        try
        {
            // 1) 释放目录：系统临时目录下的固定子目录，重复运行直接覆盖
            string dir = Path.Combine(Path.GetTempPath(), "SelfBlogProjectBook");
            Directory.CreateDirectory(dir);

            // 2) 从程序集内嵌资源读取 HTML，并写出为 UTF-8（无 BOM）文件
            string htmlPath = Path.Combine(dir, "index.html");
            using (Stream s = Assembly.GetExecutingAssembly().GetManifestResourceStream("pb.html"))
            {
                if (s == null)
                {
                    throw new InvalidOperationException("未找到内嵌的 HTML 资源。");
                }
                using (StreamReader r = new StreamReader(s, System.Text.Encoding.UTF8, true))
                {
                    // StreamReader(true) 自动识别 BOM；写出时显式指定 UTF-8 无 BOM，避免中文乱码
                    File.WriteAllText(htmlPath, r.ReadToEnd(), new System.Text.UTF8Encoding(false));
                }
            }

            // 3) 校验模式：--verify 只释放资源并写标记文件，不打开浏览器（供自动化检查用）
            if (args.Length == 1 && args[0] == "--verify")
            {
                File.WriteAllText(Path.Combine(dir, "verify-ok.txt"),
                    DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss"));
                return;
            }

            // 4) 用系统默认浏览器打开：UseShellExecute=true 走 Windows Shell 关联，无需指定浏览器路径
            Process.Start(new ProcessStartInfo(htmlPath) { UseShellExecute = true });
        }
        catch (Exception ex)
        {
            // 失败时给出可见提示，而不是静默退出
            MessageBox.Show("打开项目书失败：" + ex.Message, "全栈个人博客系统项目书");
        }
    }
}
