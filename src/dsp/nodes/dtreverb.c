#include "lib/dattorro/dattorro.h"
#include "../node.h"

static const char *cmd_strings[] = {
    "pre_delay",
    "bandwidth",
    "input_diffusion_1",
    "input_diffusion_2",
    "decay",
    "decay_diffusion_1",
    "decay_diffusion_2",
    "damping",
    "excursion_rate",
    "excursion_depth",
    "wet",
    "dry",
    NULL,
};


typedef struct {
  Node node;
  DattorroReverb dt;
  float buf[NODE_BUFFER_SIZE * 2];
  NodePort inl, inr;   /* inlets */
  NodePort outl, outr; /* outlets */
} DtReverbNode;


static void process(Node *node) {
  DtReverbNode *n = (DtReverbNode*) node;

  /* copy inlets to buffer */
  for (int i = 0; i < NODE_BUFFER_SIZE; i++) {
    n->buf[i*2+0] = n->inl.buf[i];
    n->buf[i*2+1] = n->inr.buf[i];
  }

  // Process the audio through the reverb
  dattorro_reverb_process(&n->dt, n->buf, NODE_BUFFER_SIZE * 2);

  /* copy buffer to outlets */
  for (int i = 0; i < NODE_BUFFER_SIZE; i++) {
    n->outl.buf[i] = n->buf[i*2+0];
    n->outr.buf[i] = n->buf[i*2+1];
  }

  /* send output */
  node_process(node);
}


static int receive(Node *node, const char *msg, char *err) {
  DtReverbNode *n = (DtReverbNode*) node;

  char cmd[16] = "";
  float val = 0;

  sscanf(msg, "%15s %f", cmd, &val);
  int prm = string_to_enum(cmd_strings, cmd);
  if (prm < 0) { sprintf(err, "bad command '%s'", cmd); return -1; }
  val = clampf(val, 0.0, 1.0);

  switch (prm) {
//     case ROOMSIZE : fv_set_roomsize (&n->fv, val); break;
//     case DAMP     : fv_set_damp     (&n->fv, val); break;
//     case WET      : fv_set_wet      (&n->fv, val); break;
//     case DRY      : fv_set_dry      (&n->fv, val); break;
//     case WIDTH    : fv_set_width    (&n->fv, val); break;

    case BANDWIDTH  : dattorro_reverb_set_parameter  (&n->dt, BANDWIDTH, val); break;

  }

  return 0;
}


Node* new_dtreverb_node(void) {
  DtReverbNode *node = calloc(1, sizeof(DtReverbNode));

  static const char *inlets[] = { "left", "right", NULL };
  static const char *outlets[] = { "left", "right", NULL };

  static NodeInfo info = {
    .name = "dtreverb",
    .inlets = inlets,
    .outlets = outlets,
  };

  static NodeVtable vtable = {
    .process = process,
    .receive = receive,
    .free = node_free,
  };

  node_init(&node->node, &info, &vtable, &node->inl, &node->outl);
  dattorro_reverb_init(&node->dt, NODE_SAMPLERATE);

  return &node->node;
}
