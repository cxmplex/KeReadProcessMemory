# KeReadProcessMemory

# How to Use
Create a user-land app that implements DeviceIOControl, then send the IOCTL request.

# Writing Memory
If you swap source and target, you can instead write to that area in memory. MmCopyVirtualMemory states "copies bytes from one address to another". Instead of writing to the current buffer, instead just write to another program's memory space. This will be useful when wanting to inject shellcode. Simply reverse until you find a function that is called frequently. Or even with a bit of work, inject a module into that process.
