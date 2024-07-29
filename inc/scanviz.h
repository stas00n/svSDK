/*
Scanviz serial protocol definitions
*/
#ifndef __SCANVIZ_SERIAL__
#define __SCANVIZ_SERIAL__

#include <stdint.h>

/* ������ */
#define SCANVIZ_VERSION             ((uint8_t)0)
#define SCANVIZ_START_MARKER        ((uint16_t)0x534Du)

/* �������� ����������� ���������� */
#define DISTANCE_SRC_DISABLE    0               /* ��������� */
#define DISTANCE_SRC_WHEEL      ((uint8_t)1)    /* ������ �������� */
#define DISTANCE_SRC_INSPVA     ((uint8_t)2)    /* INSPVA */ 
#define DISTANCE_SRC_GPRMC      ((uint8_t)3)    /* GPRMC */
#define DISTANCE_SRC_BESTPOSVEL ((uint8_t)4)    /* BESTPOS + BESTVEL */

/* ����� �������� */
#define ODO_MODE_DISABLE        0
#define ODO_MODE_UP           ((uint8_t)1)      /* ���� � ���� �������, �� ������ A */
#define ODO_MODE_UPDOWN       ((uint8_t)2)      /* ���������������, 2 ������ A+B */

	typedef enum class _svID {
		/* Message IDs */
		MSG_ACK = 0,
		MSG_IMUDATA = 1,
		MSG_EVENT = 2,
		MSG_ODOMETER = 3,
		MSG_LOG = 4,
		MSG_TIMEDWHEELDATA = 5,

		/* Command IDs */
		CMD_PING = 0x80,
		CMD_CAM_TRIG_TIMED = 0x81,
		CMD_PWRCTRL = 0x82,
		CMD_CAM_TRIG_DIST = 0x83,
		CMD_SET_ODO_PARAMS = 0x84,
		CMD_SET_UART_SPEED = 0x85,
		CMD_SET_WHEEL_PARAMS = 0x87,
		CMD_GET_VERSION = 0x88,
		CMD_SET_EVENT_COUNTER = 0x89,
		CMD_REBOOT = 0xEF,
		CMD_START_BOOTLOADER = 0xFE,

		/* Bootloader commands */
		BLCMD_RUNAPP = 0xF9,
		//BLCMD_RESET_KEYINDX = 0xFA,
		BLCMD_VERIFY = 0xFB,
		BLCMD_WRITE = 0xFC,
		BLCMD_ERASE = 0xFD,
	}svID_t;

#pragma pack(push, 2)
/* ��������� ������ */
typedef struct {
	uint16_t  sync;       /* Message start marker, ASCII "MS" - 0x534D */
	uint8_t   ver;        /* protocol version - reserved  */
	uint8_t   msgID;      /* message/command ID */
	uint16_t  dlen;       /* body length */
	uint16_t  resv;       /* reserved */
}scanvizHdr_t;

/* IMU ������ - ���������� ����� - ������������� � RoadLab ��� ������������ SBG */

typedef struct {
	double time;      /* UTC time, s */
	uint32_t    week;
	struct {
		float x, y, z;
	}acc;             /* Accels, m/s^2 */
	struct {
		float x, y, z;
	}gyro;            /* Gyros, rad/s */
	struct {
		float x, y, z;
	}deltaVel;        /* Delta velocity, m/s */
	struct {
		float x, y, z;
	}deltaAng;        /* Delta angle, rad */
	uint16_t      clkStatus;
}rawimu_t;


typedef struct {
	scanvizHdr_t  hdr;
	rawimu_t      imudata;
	uint16_t      chkSum;
}msgImu_t;

typedef struct {
	scanvizHdr_t  hdr;
	double        time;			/* UTC time, s */
	uint32_t	  counter;		/* ������� */
	uint16_t      srcID;		/* Event source ID */
	uint16_t      chkSum;
}msgEvent_t;

/* ������� - ������ ������ */
typedef struct {
	scanvizHdr_t  hdr;
	double        time;       /* UTC time, s */
	int32_t       pulses;     /* odometer pulses */
	uint16_t      chkSum;
}msgOdo_t;



/* �������� ��������� �������� (����� TIMEDWHEELDATA Novatel) */
typedef struct {
	scanvizHdr_t  hdr;
	double        timestamp;		/* Second of week */
	uint16_t      ticksPerRev;		/* Number of ticks per revolution */
	uint16_t      wheelVel;			/* Wheel velocity in counts/s */
	float         fWheelVel;		/* Float wheel velocity in counts/s */
	int32_t       cumulativeTicks;	/* Number of ticks */
	uint16_t      chkSum;
}msgTimedWheel_t;


/* ������������� � ��������� ������ ��������� ������� */

typedef struct {
	scanvizHdr_t  hdr;
	uint32_t      errorCode;       /* ��������� ��������� ������� 0 = OK, ��� ��������� ��� ������ */
	uint16_t      chkSum;
}msgAck_t;


/* ���� ������ ��� ACK */
typedef enum {
	OK = 0,
	UNKNOWN_CMD = 1,
	WRONG_VALUE = 2
}ackError_t;


/* ������� ���������� ��������� ������ �� ������� */

typedef struct {
	scanvizHdr_t  hdr;
	uint16_t      trgPeriod_ms;   /* ������ � ��, 0 - ���������� */
	uint16_t      trgPulse_ms;    /* ������������ �������� 0 - ��������� */
	uint16_t      chkSum;
}cmdCamTrigTimed_t;


/* ������� ���������� �������� ������� */
typedef struct {
	scanvizHdr_t  hdr;
	uint16_t      pwren;          /* ��� 0 - ��������� velodyne */
	uint16_t      chkSum;
}cmdPwrCtrl_t;

/* ������� ���������� ��������� ������ �� ����������� ���������� */
typedef struct {
	scanvizHdr_t  hdr;            /* ��������� */
	uint8_t       distSrc;        /* �������� ������ � ���������� ���������� */
	uint8_t       maxRate;        /* ����������� ������� (�������� fps ������) */
	uint16_t      trgPulse_ms;    /* ������������ �������� 0 - ��������� */
	float         trgDistance;    /* ��������� ��������� � ������ */
	uint16_t      chkSum;
}cmdCamTrigDist_t;


/* ������� ��������� ���������� ������ (��� � Novatel) */
typedef struct {
	scanvizHdr_t  hdr;            /* ��������� */
	uint16_t      ticksPerRev;    /* Number of ticks per revolution */
	uint16_t      reserved;       /* ������������ */
	double        wheelCirc;      /* ����� ���������� ������ */
	double        tickSpacing;    /* Spacing of ticks (used to weight the wheel sensor) */
	uint16_t      chkSum;
}cmdSetWheelParams_t;



/* ��������� �������� */
typedef struct {
	scanvizHdr_t  hdr;    /* ��������� */
	/* ����� ����� ��������: 0 - ������������� - ���� � ���� ������� (�� ���������),
	   1 - ������������� - ���� ������/����� */
	uint8_t       countMode;
	/* ��������� ���������� (���� �� ����� �������): 0 - ��������� (�� ���������);
	   1 - �������� */
	uint8_t       doubleResolition;
	uint8_t       outputRate;     /* ������� �������� ��������� 0 - 100 Hz */
	uint8_t       reverseDir;     /* ����������� ������ ����������� */
	uint16_t      chkSum;
}cmdSetOdoParams_t;



/* ������� ��������� �������� UART */
typedef struct {
	scanvizHdr_t  hdr;            /* ��������� */
	uint8_t       uartx;          /* ����� UART �� �����. ���������� �������� 1...6 */
	uint8_t       reserved[3];    /* �� ������������ (��������� ��� ������������ �� 4) */
	uint32_t      speed;          /* �������� UART: ���. 2400, ����. 2000000 bps */
	uint16_t      chkSum;
}cmdSetUARTSpeed_t;


/* ������ ������ firmware */
typedef struct {
	scanvizHdr_t  hdr;
	uint32_t      reserved;
	uint16_t      chkSum;
}cmdGetVersion_t;

typedef struct {
	scanvizHdr_t	hdr;
	uint16_t		reserved;
	uint16_t		eventSrcID;
	uint32_t		newCounterValue;
	uint16_t		chkSum;
}cmdSetEventCounter_t;

/* Empty command For connection test */
typedef struct {
	scanvizHdr_t	hdr;
	uint16_t		chkSum;
}cmdPing_t;
typedef struct {
	scanvizHdr_t	hdr;
	uint32_t		reserved;
	uint16_t		chkSum;
}cmdBootloader_t;

typedef struct {
	scanvizHdr_t	hdr;
	uint32_t		rebootcode;
	uint16_t		chkSum;
}cmdReboot_t;
#pragma pack (pop)


/* ������ ������� ����� ���� ��������� */
#define BODYLENGTH(svtype) ((const uint16_t)((sizeof(svtype) - sizeof(scanvizHdr_t) - 2)))



#ifdef __cplusplus
extern "C" {
#endif



#ifdef __cplusplus
}
#endif

#endif /* __SCANVIZ_SERIAL__ */