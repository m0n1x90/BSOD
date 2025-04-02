#include <ntddk.h>

#define IOCTL_TRIGGER_BSOD CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

NTSTATUS DriverIoControlHandler(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    NTSTATUS status = STATUS_SUCCESS;
    unsigned int CUSTOM_BUG_CHECK_CODE;

    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_TRIGGER_BSOD:

        if (irpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(CUSTOM_BUG_CHECK_CODE)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        if (!Irp->AssociatedIrp.SystemBuffer) {
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        RtlCopyMemory(&CUSTOM_BUG_CHECK_CODE, Irp->AssociatedIrp.SystemBuffer, sizeof(CUSTOM_BUG_CHECK_CODE));
        DbgPrint("[+] Triggering BSOD Bomb with code: 0x%X\n", CUSTOM_BUG_CHECK_CODE);
        KeBugCheckEx(CUSTOM_BUG_CHECK_CODE, 0x0, 0x0, 0x0, 0x0);

    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

NTSTATUS DriverCreateCloseHandler(
    PDEVICE_OBJECT DeviceObject, 
    PIRP IrpPacket
)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(IrpPacket);

    if (stack->MajorFunction == IRP_MJ_CREATE) {
        DbgPrint("[+] Driver opened by a user application.\n");
    }
    else if (stack->MajorFunction == IRP_MJ_CLOSE) {
        DbgPrint("[+] Driver handle closed by the user application.\n");
    }

    IrpPacket->IoStatus.Status = STATUS_SUCCESS;
    IrpPacket->IoStatus.Information = 0;

    IoCompleteRequest(IrpPacket, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

void DriverUnload(PDRIVER_OBJECT DriverObject)
{
    UNICODE_STRING symbolicLinkName;
    RtlInitUnicodeString(&symbolicLinkName, L"\\DosDevices\\BSODDriver");

    IoDeleteSymbolicLink(&symbolicLinkName);
    IoDeleteDevice(DriverObject->DeviceObject);

    DbgPrint("[+] Driver Unloaded.\n");
}

NTSTATUS DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
)
{
    UNREFERENCED_PARAMETER(RegistryPath);

    NTSTATUS status;
    
    UNICODE_STRING deviceName;
    PDEVICE_OBJECT deviceObject = NULL;
    RtlInitUnicodeString(&deviceName, L"\\Device\\BSODDriver");
    status = IoCreateDevice(
        DriverObject,
        0,
        &deviceName,
        FILE_DEVICE_UNKNOWN,
        0,
        FALSE,
        &deviceObject
    );
    if (!NT_SUCCESS(status)) {
        DbgPrint("[!] Failed to create device: 0x%X\n", status);
        return status;
    }

    UNICODE_STRING symbolicLinkName;
    RtlInitUnicodeString(&symbolicLinkName, L"\\??\\BSODDriver");    
    status = IoCreateSymbolicLink(&symbolicLinkName, &deviceName);
    if (!NT_SUCCESS(status)) {
        DbgPrint("[!] Failed to create symbolic link: 0x%X\n", status);
        IoDeleteDevice(deviceObject);
        return status;
    }

    DbgPrint("[+] BSOD Driver Loaded.\n");

    DriverObject->DriverUnload = DriverUnload;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = DriverCreateCloseHandler;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverCreateCloseHandler;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DriverIoControlHandler;

    return STATUS_SUCCESS;
}