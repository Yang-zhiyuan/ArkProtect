#include "main.h"

DYNAMIC_DATA	g_DynamicData = { 0 };
PDRIVER_OBJECT  g_DriverObject = NULL;      // ����ȫ����������

NTSTATUS
DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegisterPath)
{
	NTSTATUS       Status = STATUS_SUCCESS;
	PDEVICE_OBJECT DeviceObject = NULL;

	UNICODE_STRING uniDeviceName = { 0 };
	UNICODE_STRING uniLinkName = { 0 };

	RtlInitUnicodeString(&uniDeviceName, DEVICE_NAME);
	RtlInitUnicodeString(&uniLinkName, LINK_NAME);

	// �����豸����
	Status = IoCreateDevice(DriverObject, 0, &uniDeviceName, FILE_DEVICE_ARKPROTECT, 0, FALSE, &DeviceObject);
	if (NT_SUCCESS(Status))
	{
		// �����豸������
		Status = IoCreateSymbolicLink(&uniLinkName, &uniDeviceName);
		if (NT_SUCCESS(Status))
		{
			Status = APInitializeDynamicData(&g_DynamicData);			// ��ʼ����Ϣ
			if (NT_SUCCESS(Status))
			{
				for (INT32 i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
				{
					DriverObject->MajorFunction[i] = APDefaultPassThrough;
				}

				DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = APIoControlPassThrough;

				DriverObject->DriverUnload = APUnloadDriver;

				g_DriverObject = DriverObject;

				DbgPrint("ArkProtect is Starting!!!");
			}
			else
			{
				DbgPrint("Initialize Dynamic Data Failed\r\n");
			}
		}
		else
		{
			DbgPrint("Create SymbolicLink Failed\r\n");
		}
	}
	else
	{
		DbgPrint("Create Device Object Failed\r\n");
	}

	return Status;
}


/************************************************************************
*  Name : APInitDynamicData
*  Param: DynamicData			��Ϣ
*  Ret  : NTSTATUS
*  ��ʼ����Ϣ
************************************************************************/
NTSTATUS
APInitializeDynamicData(IN OUT PDYNAMIC_DATA DynamicData)
{
	NTSTATUS				Status = STATUS_SUCCESS;
	RTL_OSVERSIONINFOEXW	VersionInfo = { 0 };
	VersionInfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);

	if (DynamicData == NULL)
	{
		return STATUS_INVALID_ADDRESS;
	}

	RtlZeroMemory(DynamicData, sizeof(DYNAMIC_DATA));

	// ��ü�����汾��Ϣ
	Status = RtlGetVersion((PRTL_OSVERSIONINFOW)&VersionInfo);
	if (NT_SUCCESS(Status))
	{
		UINT32 Version = (VersionInfo.dwMajorVersion << 8) | (VersionInfo.dwMinorVersion << 4) | VersionInfo.wServicePackMajor;
		DynamicData->WinVersion = (eWinVersion)Version;

		DbgPrint("%x\r\n", DynamicData->WinVersion);

		switch (Version)
		{
		case WINVER_7:
		case WINVER_7_SP1:
		{
#ifdef _WIN64
			//////////////////////////////////////////////////////////////////////////
			// EProcess
			DynamicData->ThreadListHead_KPROCESS = 0x030;
			DynamicData->ObjectTable = 0x200;
			DynamicData->SectionObject = 0x268;
			DynamicData->InheritedFromUniqueProcessId = 0x290;
			DynamicData->ThreadListHead_EPROCESS = 0x308;

			DynamicData->MaxUserSpaceAddress = 0x000007FFFFFFFFFF;

#else

#endif
			break;
		}
		default:
			break;
		}

	}

	return Status;
}


NTSTATUS
APDefaultPassThrough(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}


VOID
APUnloadDriver(IN PDRIVER_OBJECT DriverObject)
{

	UNICODE_STRING  uniLinkName;
	PDEVICE_OBJECT	NextDeviceObject = NULL;
	PDEVICE_OBJECT  CurrentDeviceObject = NULL;
	RtlInitUnicodeString(&uniLinkName, LINK_NAME);

	IoDeleteSymbolicLink(&uniLinkName);		// ɾ��������
	CurrentDeviceObject = DriverObject->DeviceObject;
	while (CurrentDeviceObject != NULL)		// ѭ������ɾ���豸��
	{
		NextDeviceObject = CurrentDeviceObject->NextDevice;
		IoDeleteDevice(CurrentDeviceObject);
		CurrentDeviceObject = NextDeviceObject;
	}

	DbgPrint("ArkProtect is stopped!!!");
}