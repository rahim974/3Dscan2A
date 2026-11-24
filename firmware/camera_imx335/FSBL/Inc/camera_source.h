#ifndef CAMERA_SOURCE_H
#define CAMERA_SOURCE_H

#include <stdbool.h>
#include <stdint.h>
#include "camera_types.h"

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

#endif
