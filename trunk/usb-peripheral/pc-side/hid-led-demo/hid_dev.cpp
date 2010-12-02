/****************************************************************************
 *
 *            Copyright (c) 2006-2007 by CMX Systems, Inc.
 *
 * This software is copyrighted by and is the sole property of
 * CMX.  All rights, title, ownership, or other interests
 * in the software remain the property of CMX.  This
 * software may only be used in accordance with the corresponding
 * license agreement.  Any unauthorized use, duplication, transmission,
 * distribution, or disclosure of this software is expressly forbidden.
 *
 * This Copyright notice may not be removed or modified without prior
 * written consent of CMX.
 *
 * CMX reserves the right to modify this software without notice.
 *
 * CMX Systems, Inc.
 * 12276 San Jose Blvd. #511
 * Jacksonville, FL 32223
 * USA
 *
 * Tel:  (904) 880-1840
 * Fax:  (904) 880-1632
 * http: www.cmx.com
 * email: cmx@cmx.com
 *
 ***************************************************************************/
#include "stdafx.h"
#include <atlstr.h>
#include <setupapi.h>
#include <malloc.h>

extern "C" {
#include "hid-lib/hidsdi.h"
#include "hid-lib/hidpi.h"
}

#include "hid_dev.h"

static HANDLE hid_dev=NULL;
static DWORD OutputReportByteLength;
static DWORD InputReportByteLength;
static CString msg;
static char *obuf;
static char *ibuf;

HANDLE read_done=0;

static OVERLAPPED ovl = {
       0, 0  //Internal, Internal high
     , 0, 0  //offset, offset high
     , 0     //hEvent
  };

void HIDClose(void)
{
  if (hid_dev != INVALID_HANDLE_VALUE)
  {
    CloseHandle(hid_dev);
    hid_dev = INVALID_HANDLE_VALUE;
  }
  if (obuf != NULL)
  {
    free(obuf);
    obuf=NULL;
  }
  if (ibuf != NULL)
  {
    free(ibuf);
    ibuf=NULL;
  }
  if (read_done)
  {
    CloseHandle(read_done);
    read_done=0;
  }
}

int HIDOpen(void)
{
  const int vid=0xc1ca, pid=0x0003;     /* Vendor and product ID of the device. */
  int r=0;

  obuf=ibuf=NULL;

  BOOLEAN GotRequiredSize=FALSE;
  DWORD Index, DataSize;;
  DWORD RequiredSize;

  PSP_DEVICE_INTERFACE_DETAIL_DATA detailData = NULL; 


  SECURITY_ATTRIBUTES   Hid_Security = {sizeof(SECURITY_ATTRIBUTES), NULL, true};

 


  /* 1) Get the HID Globally Unique ID from the OS */
  GUID HidGuid;
  HidD_GetHidGuid(&HidGuid);


  /* 2) Get an array of structures containing information about
  all attached and enumerated HIDs */
  HDEVINFO HidDevInfo;
  HidDevInfo = SetupDiGetClassDevs(	&HidGuid, 
									  NULL, 
									  NULL, 
									  DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);


  /* 3) Step through the attached device list 1 by 1 and examine
  each of the attached devices.  When there are no more entries in
  the array of structures, the function will return error. */
  
  SP_DEVICE_INTERFACE_DATA devInfoData;
  Index = 0;									/* init to first index of array */
  devInfoData.cbSize = sizeof(devInfoData);	/* set to the size of the structure
											  that will contain the device info data */

  do {
    BOOLEAN Result;
	  /* Get information about the HID device with the 'Index' array entry */
	  Result = SetupDiEnumDeviceInterfaces(	HidDevInfo, 
											  0, 
											  &HidGuid, 
											  Index, 
											  &devInfoData);
    
	  /* If we run into this condition, then there are no more entries
	  to examine, we might as well return FALSE at point */
	  if(Result == FALSE)
	  {
      r=1;
      msg="Device not found.";
      break;    	
	  }

	  if(GotRequiredSize == FALSE)
	  {
		  /* 3) Get the size of the DEVICE_INTERFACE_DETAIL_DATA
		  structure.  The first call will return an error condition, 
		  but we'll get the size of the strucure */
		  Result = SetupDiGetDeviceInterfaceDetail(	HidDevInfo,
													  &devInfoData,
													  NULL,
													  0,
													  &DataSize,
													  NULL);
		  GotRequiredSize = TRUE;

		  /* allocate memory for the HidDevInfo structure */
		  detailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA) _alloca(DataSize);
    	
		  /* set the size parameter of the structure */
		  detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
	  }
    		   	
	  /* 4) Now call the function with the correct size parameter.  This 
	  function will return data from one of the array members that 
	  Step #2 pointed to.  This way we can start to identify the
	  attributes of particular HID devices.  */
	  Result = SetupDiGetDeviceInterfaceDetail(	HidDevInfo,
												  &devInfoData,
												  detailData,
												  DataSize,
												  &RequiredSize,
												  NULL);
    	
	  /* 5) Open a file handle to the device.  Make sure the
	  attibutes specify overlapped transactions or the IN
	  transaction may block the input thread. */
	  hid_dev = CreateFile( detailData->DevicePath,
								  GENERIC_READ | GENERIC_WRITE, /* read / write access*/
								  0,                            /* exclusive access */
								   &Hid_Security,               /* Security */
								  OPEN_EXISTING,               
								  FILE_FLAG_OVERLAPPED,         /* overlapped I/O */
								  NULL);                        /* no template */

    	
	  /* 6) Get the Device VID & PID to see if it's the device we want */
	  if(hid_dev != INVALID_HANDLE_VALUE)
	  {
      HIDD_ATTRIBUTES HIDAttrib;
		  HIDAttrib.Size = sizeof(HIDAttrib);
		  HidD_GetAttributes(	hid_dev, &HIDAttrib);

		  if((HIDAttrib.VendorID == vid) && (HIDAttrib.ProductID == pid))
		  { /* Found HID device. */

        /* get a handle to a buffer that describes the device's capabilities.  This
	      line plus the following two lines of code extract the report length the
	      device is claiming to support */
        PHIDP_PREPARSED_DATA hpd;
	      HidD_GetPreparsedData(hid_dev, &hpd);
      	
	      /* extract the capabilities info */
        HIDP_CAPS		hid_cap;
	      HidP_GetCaps(hpd , &hid_cap);
      	
	      /* Free the memory allocated when getting the preparsed data */
	      HidD_FreePreparsedData(hpd);		
        /* This will be needed by writes. */
        OutputReportByteLength = hid_cap.OutputReportByteLength;
        obuf=(char *)malloc(OutputReportByteLength);
        /* and this by reads */
        InputReportByteLength=hid_cap.InputReportByteLength;
        ibuf=(char *) malloc(InputReportByteLength);
        /* create a new event for overlapped read.  */
        read_done=CreateEvent(0, false, true, NULL);
        r=0;
        break;
		  }
    	
		  /* 7) Close the Device Handle because we didn't find the device
		  with the correct VID and PID */
		  CloseHandle(hid_dev);
	  }

	  Index++;	/* increment the array index to search the next entry */

  } while(1);

	/* free HID device info list resources */
	SetupDiDestroyDeviceInfoList(HidDevInfo);

	return(r);
}

void HIDWrite(void* data)
{
  DWORD written;
  
  OVERLAPPED ovl = {
      0, 0  //Internal, Internal high
    , 0, 0  //offset, offset high
    , 0     //hEvent
  };

  /* The firs byte is the report identifyer. This byte will not be sent. */
  obuf[0]=0;
  /* put data bytes into the buffer */
  memcpy(&obuf[1], data, OutputReportByteLength-1);

  WriteFile(hid_dev, obuf, OutputReportByteLength, &written, &ovl);
  
  GetOverlappedResult(hid_dev, &ovl, &written, true);

  if (written != OutputReportByteLength)
  {
    LPVOID lpMsgBuf;
    DWORD dw = GetLastError(); 
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );
    msg=__FUNCTION__": write error! (";
    msg += (char*)lpMsgBuf;
    msg + ")";
    LocalFree(lpMsgBuf);
    throw(msg.GetString());
  }
}

int HIDRead(void* data)
{
  DWORD readed;
  static int rd_state=0;
  static void *dst=data;

  switch(rd_state)
  {
  case 0:  /* start read */
    ibuf[0]=0;
    ReadFile(hid_dev, ibuf, InputReportByteLength, &readed, &ovl);
    rd_state=1;
  case 1:  /* check if read has been finished */
    if (!HasOverlappedIoCompleted(&ovl))
    {
      return(0);
    }
    GetOverlappedResult(hid_dev, &ovl, &readed, true);
    if (InputReportByteLength != readed)
    {
      LPVOID lpMsgBuf;
      DWORD dw = GetLastError(); 
      FormatMessage(
          FORMAT_MESSAGE_ALLOCATE_BUFFER | 
          FORMAT_MESSAGE_FROM_SYSTEM,
          NULL,
          dw,
          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
          (LPTSTR) &lpMsgBuf,
          0, NULL );
      msg=__FUNCTION__": write error! (";
      msg+=(char*)lpMsgBuf;
      msg+=")";
      LocalFree(lpMsgBuf);
      throw(msg.GetString());
    }
    memcpy(dst, &ibuf[1], InputReportByteLength-1);
    rd_state=0;
    return(readed);
    break;
  }    
  return(0);
}
