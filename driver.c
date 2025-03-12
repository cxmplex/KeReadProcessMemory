#include < ntifs.h > 
#include < ntddk.h > 
#include < ntstrsafe.h > 
#include < stdlib.h >

#define IOCTL_DUMP_MEM CTL_CODE(FILE_DEVICE_UNKNOWN, 0x999, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)

PDEVICE_OBJECT pDeviceObject;
UNICODE_STRING dev, dos;

DRIVER_DISPATCH Create;
DRIVER_DISPATCH IOCTL;
DRIVER_DISPATCH Close;
DRIVER_UNLOAD Unload;

void Unload(PDRIVER_OBJECT pDriverObject) {
    IoDeleteSymbolicLink( & dos);
    IoDeleteDevice(pDriverObject - > DeviceObject);
}

NTSTATUS Create(PDEVICE_OBJECT DeviceObject,PIRP irp)
{
	irp->IoStatus.Status=STATUS_SUCCESS;
	irp->IoStatus.Information=0;

	IoCompleteRequest(irp,IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS Close(PDEVICE_OBJECT DeviceObject,PIRP irp)
{
	irp->IoStatus.Status=STATUS_SUCCESS;
	irp->IoStatus.Information=0;

	IoCompleteRequest(irp,IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

struct {
    int PID;
    void * Addr;
    void * Value;
    int Bytes;
}
MemReq;

NTSTATUS NTAPI MmCopyVirtualMemory
    (
        PEPROCESS SourceProcess,
        PVOID SourceAddress,
        PEPROCESS TargetProcess,
        PVOID TargetAddress,
        SIZE_T BufferSize,
        KPROCESSOR_MODE PreviousMode,
        PSIZE_T ReturnSize
    );

NTSTATUS KeReadProcessMemory(HANDLE PID, PVOID SourceAddress, PVOID TargetAddress, SIZE_T Size) {
    SIZE_T Result;
    PEPROCESS SourceProcess, TargetProcess;
    PsLookupProcessByProcessId(PID, & SourceProcess);
    TargetProcess = PsGetCurrentProcess();
    __try {
        //ProbeForRead(SourceAddress, Size, 1);
        MmCopyVirtualMemory(SourceProcess, SourceAddress, TargetProcess, TargetAddress, Size, KernelMode, & Result);
        return STATUS_SUCCESS;
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        return STATUS_ACCESS_DENIED;
    }
}

NTSTATUS IOCTL(PDEVICE_OBJECT DeviceObject, PIRP irp) {
    PUCHAR UserBuffer;
    PIO_STACK_LOCATION io;

    io = IoGetCurrentIrpStackLocation(irp);
    switch (io - > Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_DUMP_MEM:
        memcpy( & MemReq, irp - > AssociatedIrp.SystemBuffer, sizeof(MemReq));
        UserBuffer = (PUCHAR) MmGetSystemAddressForMdlSafe(irp - > MdlAddress, NormalPagePriority);
        if (UserBuffer && MemReq.Addr != NULL) {
                KeReadProcessMemory((HANDLE) MemReq.PID, MemReq.Addr, (PVOID) UserBuffer, MemReq.Bytes);
        }
        KeFlushIoBuffers(irp - > MdlAddress, TRUE, FALSE);
        irp - > IoStatus.Information = 0;
        break;
    default:

        irp - > IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        irp - > IoStatus.Information = 0;

        IoCompleteRequest(irp, IO_NO_INCREMENT);
        return STATUS_INVALID_DEVICE_REQUEST;
    }
    irp - > IoStatus.Status = STATUS_SUCCESS;
    irp - > IoStatus.Information = 0;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath) {
    RtlInitUnicodeString( & dev, L "\\Device\\GITHUB_ComplexityDev");
    RtlInitUnicodeString( & dos, L "\\DosDevices\\ComplexityDev");

    IoCreateDevice(pDriverObject, 0, & dev, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, & pDeviceObject);
    IoCreateSymbolicLink( & dos, & dev);

    IoSetDeviceInterfaceState(pRegistryPath, TRUE);

    pDriverObject - > MajorFunction[IRP_MJ_CREATE] = Create;
    pDriverObject - > MajorFunction[IRP_MJ_DEVICE_CONTROL] = IOCTL;
    pDriverObject - > MajorFunction[IRP_MJ_CLOSE] = Close;

    pDriverObject - > DriverUnload = Unload;

    pDeviceObject - > Flags |= DO_DIRECT_IO;
    pDeviceObject - > Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;
}
