/*
Scanviz serial protocol definitions
*/
#ifndef __SCANVIZ_SERIAL__
#define __SCANVIZ_SERIAL__

#include <stdint.h>

/* Разное */
#define SCANVIZ_VERSION             ((uint8_t)0)
#define SCANVIZ_START_MARKER        ((uint16_t)0x534Du)

/* Источник пройденного расстояния */
#define DISTANCE_SRC_DISABLE    0               /* Отключено */
#define DISTANCE_SRC_WHEEL      ((uint8_t)1)    /* Датчик одометра */
#define DISTANCE_SRC_INSPVA     ((uint8_t)2)    /* INSPVA */ 
#define DISTANCE_SRC_GPRMC      ((uint8_t)3)    /* GPRMC */
#define DISTANCE_SRC_BESTPOSVEL ((uint8_t)4)    /* BESTPOS + BESTVEL */

/* Режим одометра */
#define ODO_MODE_DISABLE        0
#define ODO_MODE_UP           ((uint8_t)1)      /* Счет в одну сторону, по каналу A */
#define ODO_MODE_UPDOWN       ((uint8_t)2)      /* Двунаправленный, 2 канала A+B */

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
/* Заголовок пакета */
typedef struct {
	uint16_t  sync;       /* Message start marker, ASCII "MS" - 0x534D */
	uint8_t   ver;        /* protocol version - reserved  */
	uint8_t   msgID;      /* message/command ID */
	uint16_t  dlen;       /* body length */
	uint16_t  resv;       /* reserved */
}scanvizHdr_t;

/* IMU данные - устаревший пакет - использовался в RoadLab для ретрансляции SBG */

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
	uint32_t	  counter;		/* Счетчик */
	uint16_t      srcID;		/* Event source ID */
	uint16_t      chkSum;
}msgEvent_t;

/* одометр - старая версия */
typedef struct {
	scanvizHdr_t  hdr;
	double        time;       /* UTC time, s */
	int32_t       pulses;     /* odometer pulses */
	uint16_t      chkSum;
}msgOdo_t;



/* выходное сообщение одометра (копия TIMEDWHEELDATA Novatel) */
typedef struct {
	scanvizHdr_t  hdr;
	double        timestamp;		/* Second of week */
	uint16_t      ticksPerRev;		/* Number of ticks per revolution */
	uint16_t      wheelVel;			/* Wheel velocity in counts/s */
	float         fWheelVel;		/* Float wheel velocity in counts/s */
	int32_t       cumulativeTicks;	/* Number of ticks */
	uint16_t      chkSum;
}msgTimedWheel_t;


/* Подтверждение и результат приема последней команды */

typedef struct {
	scanvizHdr_t  hdr;
	uint32_t      errorCode;       /* Результат последней команды 0 = OK, или ненулевой код ошибки */
	uint16_t      chkSum;
}msgAck_t;


/* Коды ошибок для ACK */
typedef enum {
	OK = 0,
	UNKNOWN_CMD = 1,
	WRONG_VALUE = 2
}ackError_t;


/* Команда управления триггером камеры по времени */

typedef struct {
	scanvizHdr_t  hdr;
	uint16_t      trgPeriod_ms;   /* Период в мс, 0 - однократно */
	uint16_t      trgPulse_ms;    /* Длительность импульса 0 - отключено */
	uint16_t      chkSum;
}cmdCamTrigTimed_t;


/* Команда управления питанием сканера */
typedef struct {
	scanvizHdr_t  hdr;
	uint16_t      pwren;          /* Бит 0 - включение velodyne */
	uint16_t      chkSum;
}cmdPwrCtrl_t;

/* Команда управления триггером камеры по пройденному расстоянию */
typedef struct {
	scanvizHdr_t  hdr;            /* Заголовок */
	uint8_t       distSrc;        /* Источник данных о пройденном расстоянии */
	uint8_t       maxRate;        /* Ограничение частоты (максимум fps камеры) */
	uint16_t      trgPulse_ms;    /* Длительность импульса 0 - отключено */
	float         trgDistance;    /* Требуемая дистанция в метрах */
	uint16_t      chkSum;
}cmdCamTrigDist_t;


/* Команда установки параметров колеса (как у Novatel) */
typedef struct {
	scanvizHdr_t  hdr;            /* Заголовок */
	uint16_t      ticksPerRev;    /* Number of ticks per revolution */
	uint16_t      reserved;       /* Выравнивание */
	double        wheelCirc;      /* длина окружности колеса */
	double        tickSpacing;    /* Spacing of ticks (used to weight the wheel sensor) */
	uint16_t      chkSum;
}cmdSetWheelParams_t;



/* Настройка одометра */
typedef struct {
	scanvizHdr_t  hdr;    /* Заголовок */
	/* Режим счета одометра: 0 - одноканальный - счет в одну сторону (по умолчанию),
	   1 - двухканальный - счет вперед/назад */
	uint8_t       countMode;
	/* Удвоенное разрешение (счет по обоим фронтам): 0 - отключено (по умолчанию);
	   1 - включено */
	uint8_t       doubleResolition;
	uint8_t       outputRate;     /* частота отправки сообщений 0 - 100 Hz */
	uint8_t       reverseDir;     /* Программный реверс направления */
	uint16_t      chkSum;
}cmdSetOdoParams_t;



/* Команда изменения скорости UART */
typedef struct {
	scanvizHdr_t  hdr;            /* Заголовок */
	uint8_t       uartx;          /* Номер UART на плате. Допустимые значения 1...6 */
	uint8_t       reserved[3];    /* Не используется (вставлено для выравнивания по 4) */
	uint32_t      speed;          /* Скорость UART: мин. 2400, макс. 2000000 bps */
	uint16_t      chkSum;
}cmdSetUARTSpeed_t;


/* Запрос версии firmware */
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


/* макрос расчета длины тела сообщения */
#define BODYLENGTH(svtype) ((const uint16_t)((sizeof(svtype) - sizeof(scanvizHdr_t) - 2)))



#ifdef __cplusplus
extern "C" {
#endif



#ifdef __cplusplus
}
#endif

#endif /* __SCANVIZ_SERIAL__ */