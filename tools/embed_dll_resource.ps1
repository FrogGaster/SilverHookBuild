param(
    [Parameter(Mandatory = $true)]
    [string]$LoaderPath,

    [Parameter(Mandatory = $true)]
    [string]$DllPath,

    [int]$ResourceId = 101
)

$ErrorActionPreference = "Stop"
$loader = (Resolve-Path -LiteralPath $LoaderPath).Path
$dll = (Resolve-Path -LiteralPath $DllPath).Path

if (-not ("NativeResourceUpdater" -as [type])) {
    Add-Type -TypeDefinition @"
using System;
using System.ComponentModel;
using System.IO;
using System.Runtime.InteropServices;

public static class NativeResourceUpdater
{
    private static readonly IntPtr RcData = new IntPtr(10);

    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern IntPtr BeginUpdateResourceW(string fileName, bool deleteExistingResources);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool UpdateResourceW(
        IntPtr updateHandle,
        IntPtr type,
        IntPtr name,
        ushort language,
        byte[] data,
        uint dataSize);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool EndUpdateResourceW(IntPtr updateHandle, bool discard);

    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern IntPtr LoadLibraryExW(string fileName, IntPtr file, uint flags);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern IntPtr FindResourceW(IntPtr module, IntPtr name, IntPtr type);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern uint SizeofResource(IntPtr module, IntPtr resource);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern IntPtr LoadResource(IntPtr module, IntPtr resource);

    [DllImport("kernel32.dll")]
    private static extern IntPtr LockResource(IntPtr resourceData);

    [DllImport("kernel32.dll")]
    private static extern bool FreeLibrary(IntPtr module);

    public static void Embed(string executablePath, string dllPath, int resourceId)
    {
        byte[] data = File.ReadAllBytes(dllPath);
        IntPtr handle = BeginUpdateResourceW(executablePath, false);
        if (handle == IntPtr.Zero)
            throw new Win32Exception(Marshal.GetLastWin32Error(), "BeginUpdateResourceW failed");

        bool discard = true;
        try
        {
            if (!UpdateResourceW(
                    handle,
                    RcData,
                    new IntPtr(resourceId),
                    0,
                    data,
                    (uint)data.Length))
                throw new Win32Exception(Marshal.GetLastWin32Error(), "UpdateResourceW failed");
            discard = false;
        }
        finally
        {
            if (!EndUpdateResourceW(handle, discard) && !discard)
                throw new Win32Exception(Marshal.GetLastWin32Error(), "EndUpdateResourceW failed");
        }
    }

    public static void Verify(string executablePath, string dllPath, int resourceId)
    {
        byte[] expected = File.ReadAllBytes(dllPath);
        IntPtr module = LoadLibraryExW(executablePath, IntPtr.Zero, 0x00000002);
        if (module == IntPtr.Zero)
            throw new Win32Exception(Marshal.GetLastWin32Error(), "LoadLibraryExW failed");

        try
        {
            IntPtr resource = FindResourceW(module, new IntPtr(resourceId), RcData);
            if (resource == IntPtr.Zero)
                throw new Win32Exception(Marshal.GetLastWin32Error(), "FindResourceW failed");

            uint size = SizeofResource(module, resource);
            IntPtr resourceData = LoadResource(module, resource);
            IntPtr bytes = resourceData == IntPtr.Zero ? IntPtr.Zero : LockResource(resourceData);
            if (bytes == IntPtr.Zero || size != expected.Length)
                throw new InvalidDataException("Embedded DLL size does not match the source DLL.");

            byte[] actual = new byte[size];
            Marshal.Copy(bytes, actual, 0, actual.Length);
            for (int i = 0; i < expected.Length; ++i)
            {
                if (expected[i] != actual[i])
                    throw new InvalidDataException("Embedded DLL contents do not match the source DLL.");
            }
        }
        finally
        {
            FreeLibrary(module);
        }
    }
}
"@
}

[NativeResourceUpdater]::Embed($loader, $dll, $ResourceId)
[NativeResourceUpdater]::Verify($loader, $dll, $ResourceId)
Write-Host "Embedded: $dll -> $loader (RCDATA #$ResourceId)"
