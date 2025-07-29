#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct trnr_pump trnr_pump;

trnr_pump* trnr_pump_create(void);
void trnr_pump_free(trnr_pump* pump);
void trnr_pump_process_block(trnr_pump* pump, float** audio, float** sidechain, int frames);

#ifdef __cplusplus
}
#endif
