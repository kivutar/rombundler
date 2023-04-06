#include <stdlib.h>
#include <unistd.h>
#include <ggponet.h>
#include <platform_unix.h>
#include "nongamestate.h"
#include "core.h"
#include "config.h"
#include "input.h"
#include "video.h"
#include "audio.h"

GGPOSession *ggpo = NULL;
NonGameState ngs = { 0 };
#define MAX_CHARS 2
extern config g_cfg;
extern bool audio_fast_forward;

int
fletcher32_checksum(short *data, size_t len)
{
   int sum1 = 0xffff, sum2 = 0xffff;

   while (len) {
      unsigned tlen = len > 360 ? 360 : len;
      len -= tlen;
      do {
         sum1 += *data++;
         sum2 += sum1;
      } while (--tlen);
      sum1 = (sum1 & 0xffff) + (sum1 >> 16);
      sum2 = (sum2 & 0xffff) + (sum2 >> 16);
   }

   /* Second reduction step to reduce sums to 16 bits */
   sum1 = (sum1 & 0xffff) + (sum1 >> 16);
   sum2 = (sum2 & 0xffff) + (sum2 >> 16);
   return sum2 << 16 | sum1;
}

bool __cdecl
net_begin_game_callback(const char *name)
{
   core_load(g_cfg.core);
	core_load_game(g_cfg.rom);
	// srm_load();

   return true;
}

void net_advance_frame(uint16_t inputs[], int disconnect_flags)
{
   // gs.Update(inputs, disconnect_flags);
   input_set_state(inputs);

   // update the checksums to display in the top of the window.  this
   // helps to detect desyncs.
   // ngs.now.framenumber = gs._framenumber;
   // ngs.now.checksum = fletcher32_checksum((short *)&gs, sizeof(gs) / 2);
   // if ((gs._framenumber % 90) == 0) {
   //    ngs.periodic = ngs.now;
   // }

   audio_fast_forward = ggpo_in_rollback(ggpo);

   core_run();

   // Notify ggpo that we've moved forward exactly 1 frame.
   ggpo_advance_frame(ggpo);

   // Update the performance monitor display.
   // GGPOPlayerHandle handles[MAX_PLAYERS];
   // int count = 0;
   // for (int i = 0; i < ngs.num_players; i++) {
   //    if (ngs.players[i].type == GGPO_PLAYERTYPE_REMOTE) {
   //       handles[count++] = ngs.players[i].handle;
   //    }
   // }
   // ggpoutil_perfmon_update(ggpo, handles, count);
}

void
net_idle(int time)
{
   ggpo_idle(ggpo, time);
}

void
net_run_frame()
{
   GGPOErrorCode result = GGPO_OK;
   int disconnect_flags;
   uint16_t inputs[MAX_CHARS] = { 0 };

   if (ngs.local_player_handle != GGPO_INVALID_HANDLE) {
      input_poll();
      uint16_t input = input_get_state(0);
#if defined(SYNC_TEST)
      input = rand(); // test: use random inputs to demonstrate sync testing
#endif
      result = ggpo_add_local_input(ggpo, ngs.local_player_handle, &input, sizeof(input));
      printf("input: %d\n", result);
   }

   // synchronize these inputs with ggpo.  If we have enough input to proceed
   // ggpo will modify the input list with the correct inputs to use and
   // return 1.
   if (GGPO_SUCCEEDED(result)) {
      result = ggpo_synchronize_input(ggpo, (void *)inputs, sizeof(uint16_t) * MAX_CHARS, &disconnect_flags);
      if (GGPO_SUCCEEDED(result)) {
         printf("SYNCED!!!\n");
         // inputs[0] and inputs[1] contain the inputs for p1 and p2.  Advance
         // the game by 1 frame using those inputs.
         net_advance_frame(inputs, disconnect_flags);
      }
   }
   video_render();
}

bool __cdecl
net_advance_frame_callback(int flags)
{
   uint16_t inputs[MAX_CHARS] = { 0 };
   int disconnect_flags;

   // Make sure we fetch new inputs from GGPO and use those to update
   // the game state instead of reading from the keyboard.
   ggpo_synchronize_input(ggpo, (void *)inputs, sizeof(uint16_t) * MAX_CHARS, &disconnect_flags);
   net_advance_frame(inputs, disconnect_flags);
   return true;
}

bool __cdecl
net_load_game_state_callback(unsigned char *buffer, int len)
{
   return core_unserialize(buffer, len);
}

bool __cdecl
net_save_game_state_callback(unsigned char **buffer, int *len, int *checksum, int frame)
{
   *len = core_serialize_size();
   *buffer = (unsigned char *)malloc(*len);
   if (!*buffer)
      return false;

   if (!core_serialize(*buffer, *len))
      return false;

   *checksum = fletcher32_checksum((short *)*buffer, *len / 2);
   printf("checksum: %d\n", *checksum);

   return true;
}

void __cdecl 
net_free_buffer(void *buffer)
{
   free(buffer);
}

bool __cdecl
net_on_event_callback(GGPOEvent *info)
{
   int progress;
   switch (info->code) {
   case GGPO_EVENTCODE_CONNECTED_TO_PEER:
      ngs.SetConnectState(info->u.connected.player, Synchronizing);
      break;
   case GGPO_EVENTCODE_SYNCHRONIZING_WITH_PEER:
      progress = 100 * info->u.synchronizing.count / info->u.synchronizing.total;
      ngs.UpdateConnectProgress(info->u.synchronizing.player, progress);
      break;
   case GGPO_EVENTCODE_SYNCHRONIZED_WITH_PEER:
      ngs.UpdateConnectProgress(info->u.synchronized.player, 100);
      break;
   case GGPO_EVENTCODE_RUNNING:
      ngs.SetConnectState(Running);
      // renderer->SetStatusText("");
      break;
   case GGPO_EVENTCODE_CONNECTION_INTERRUPTED:
      ngs.SetDisconnectTimeout(info->u.connection_interrupted.player,
                               Platform::GetCurrentTimeMS(),
                               info->u.connection_interrupted.disconnect_timeout);
      break;
   case GGPO_EVENTCODE_CONNECTION_RESUMED:
      ngs.SetConnectState(info->u.connection_resumed.player, Running);
      break;
   case GGPO_EVENTCODE_DISCONNECTED_FROM_PEER:
      ngs.SetConnectState(info->u.disconnected.player, Disconnected);
      break;
   case GGPO_EVENTCODE_TIMESYNC:
      usleep(1000 * info->u.timesync.frames_ahead / 60);
      break;
   }
   return true;
}

bool __cdecl
net_log_game_state(char *filename, unsigned char *buffer, int len)
{
   return true;
}
