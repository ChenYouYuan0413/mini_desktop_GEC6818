#ifndef IR_APP_H
#define IR_APP_H

void ir_app_init(void);
void ir_app_run(void);
int  ir_poll(int *action);
void ir_flush(void);

#endif
