#ifndef CAMERA_SOURCE_H
#define CAMERA_SOURCE_H

#include <stdbool.h>
#include <stdint.h>
#include "camera_types.h"

/* Pipeline materiel IMX335 / DCMIPP / ISP. */
void CameraSource_PrepareHardware(void);
void CameraSource_Start(void);
bool CameraSource_Process(void);
uint32_t CameraSource_GetFrameCount(void);

/* Appeler une fois avant le demarrage DCMIPP. */
void CameraSource_Init(const CameraFrame *initial_frame);

/* Appeler depuis le callback de fin de frame DCMIPP. */
void CameraSource_OnFrameEvent(uint32_t frame_id,
                               uint32_t timestamp_ms);

/* Lecture depuis la boucle principale uniquement. */
bool CameraSource_IsFrameReady(void);

/*
 * Retourne false si aucune nouvelle frame n'est disponible.
 * Retourne true et acquitte la derniere frame sinon.
 * Copie les metadonnees, jamais les pixels du framebuffer.
 */
bool CameraSource_GetLatestFrame(CameraFrame *out_frame);

/* Single consumer, main-loop context only. Snapshot pixels are read-only.
 * Request is accepted only when FREE. Acquire transfers ownership until
 * Release. Do not retain data pointer after Release. IDs are Pipe2-specific.
 * On hardware error/timeout, fail closed; restart firmware to recover.
 */
bool CameraSource_RequestSnapshot(void);
bool CameraSource_IsSnapshotReady(void);
bool CameraSource_AcquireSnapshot(CameraFrame *out_frame);
bool CameraSource_ReleaseSnapshot(void);
uint32_t CameraSource_GetSnapshotErrorCount(void);

#endif
