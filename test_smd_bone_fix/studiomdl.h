
#pragma once

#include "studio.h"

#define STUDIO_VERSION	10

#define IDSTUDIOHEADER	(('T'<<24)+('S'<<16)+('D'<<8)+'I')
// little-endian "IDST"
#define IDSTUDIOSEQHEADER	(('Q'<<24)+('S'<<16)+('D'<<8)+'I')
														// little-endian "IDSQ"
#define ROLL	2
#define PITCH	0
#define YAW		1

typedef struct
{
	int					bone;		// bone transformation index
	vec3_t				org;		// original position
} s_vertex_t;

typedef struct
{
	int					skinref;
	int					bone;		// bone transformation index
	vec3_t				org;		// original position
} s_normal_t;


typedef struct {
	s_vertex_t			pos;
	s_normal_t			normal;
	int					s, t;
	float				u, v;
} s_trianglevert_t;


//============================================================================


// dstudiobone_t bone[MAXSTUDIOBONES];
typedef struct
{
	vec3_t	worldorg;
	float m[3][4];
	float im[3][4];
	float length;
} s_bonefixup_t;

int numbones;
typedef struct
{
	char			name[32];	// bone name for symbolic links
	int		 		parent;		// parent bone
	int				bonecontroller;	// -1 == 0
	int				flags;		// X, Y, Z, XR, YR, ZR
	// short		value[6];	// default DoF values
	vec3_t			pos;		// default pos
	vec3_t			posscale;	// pos values scale
	vec3_t			rot;		// default pos
	vec3_t			rotscale;	// rotation values scale
	int				group;		// hitgroup
	vec3_t			bmin, bmax;	// bounding box
} s_bonetable_t;

int numrenamedbones;
typedef struct
{
	char			from[32];
	char			to[32];
} s_renamebone_t;

int numhitboxes;
typedef struct
{
	char			name[32];	// bone name
	int				bone;
	int				group;		// hitgroup
	int				model;
	vec3_t			bmin, bmax;	// bounding box
} s_bbox_t;

int numhitgroups;
typedef struct
{
	int				models;
	int				group;
	char			name[32];	// bone name
} s_hitgroup_t;

typedef struct
{
	char	name[32];
	int		bone;
	int		type;
	int		index;
	float	start;
	float	end;
} s_bonecontroller_t;

s_bonecontroller_t bonecontroller[MAXSTUDIOSRCBONES];
int numbonecontrollers;

typedef struct
{
	char	name[32];
	char	bonename[32];
	int		index;
	int		bone;
	int		type;
	vec3_t	org;
} s_attachment_t;

typedef struct
{
	char			name[64];
	int				parent;
	int				mirrored;
} s_node_t;

typedef struct
{
	char			name[64];
	int				startframe;
	int				endframe;
	int				flags;
	int				numbones;
	s_node_t		node[MAXSTUDIOSRCBONES];
	int				bonemap[MAXSTUDIOSRCBONES];
	int				boneimap[MAXSTUDIOSRCBONES];
	vec3_t* pos[MAXSTUDIOSRCBONES];
	vec3_t* rot[MAXSTUDIOSRCBONES];
	int				numanim[MAXSTUDIOSRCBONES][6];
	mstudioanimvalue_t* anim[MAXSTUDIOSRCBONES][6];
} s_animation_t;

typedef struct
{
	int				event;
	int				frame;
	char			options[64];
} s_event_t;

typedef struct
{
	int				index;
	vec3_t			org;
	int				start;
	int				end;
} s_pivot_t;

typedef struct
{
	int				motiontype;
	vec3_t			linearmovement;

	char			name[64];
	int				flags;
	float			fps;
	int				numframes;

	int				activity;
	int				actweight;

	int				frameoffset; // used to adjust frame numbers

	int				numevents;
	s_event_t		event[MAXSTUDIOEVENTS];

	int				numpivots;
	s_pivot_t		pivot[MAXSTUDIOPIVOTS];

	int				numblends;
	s_animation_t* panim[MAXSTUDIOGROUPS];
	float			blendtype[2];
	float			blendstart[2];
	float			blendend[2];

	vec3_t			automovepos[MAXSTUDIOANIMATIONS];
	vec3_t			automoveangle[MAXSTUDIOANIMATIONS];

	int				seqgroup;
	int				animindex;

	vec3_t 			bmin;
	vec3_t			bmax;

	int				entrynode;
	int				exitnode;
	int				nodeflags;
} s_sequence_t;

typedef struct {
	char	label[32];
	char	name[64];
} s_sequencegroup_t;

typedef struct {
	byte r, g, b;
} rgb_t;
typedef struct {
	byte b, g, r, x;
} rgb2_t;

// FIXME: what about texture overrides inline with loading models

typedef struct
{
	char	name[64];
	int		flags;
	int		srcwidth;
	int		srcheight;
	byte* ppicture;
	rgb_t* ppal;

	float	max_s;
	float   min_s;
	float	max_t;
	float	min_t;
	int		skintop;
	int		skinleft;
	int		skinwidth;
	int		skinheight;
	float	fskintop;
	float	fskinleft;
	float	fskinwidth;
	float	fskinheight;

	int		size;
	void* pdata;

	int		parent;
} s_texture_t;

typedef struct
{
	int alloctris;
	int numtris;
	s_trianglevert_t (*triangle)[3];

	int skinref;
	int numnorms;
} s_mesh_t;

typedef struct
{
	vec3_t			pos;
	vec3_t			rot;
} s_bone_t;

typedef struct s_triangle_s
{
	s_trianglevert_t triverts[3];
	int texture_id;
} s_triangle_t;


typedef struct s_model_s
{
	char name[128];

	int numbones;
	s_node_t node[MAXSTUDIOSRCBONES];
	s_bone_t skeleton[MAXSTUDIOSRCBONES];

	s_triangle_s* trimesh; //[MAXSTUDIOTRIANGLES] ;
	int numtriangles;
} s_model_t;


typedef struct
{
	char				name[32];
	int					nummodels;
	int					base;
	s_model_t* pmodel[MAXSTUDIOMODELS];
} s_bodypart_t;



#define DotProduct(x,y) ((x)[0]*(y)[0]+(x)[1]*(y)[1]+(x)[2]*(y)[2])
#define VectorFill(a,b) { (a)[0]=(b); (a)[1]=(b); (a)[2]=(b);}
#define VectorAvg(a) ( ( (a)[0] + (a)[1] + (a)[2] ) / 3 )
#define VectorSubtract(a,b,c) {(c)[0]=(a)[0]-(b)[0];(c)[1]=(a)[1]-(b)[1];(c)[2]=(a)[2]-(b)[2];}
#define VectorAdd(a,b,c) {(c)[0]=(a)[0]+(b)[0];(c)[1]=(a)[1]+(b)[1];(c)[2]=(a)[2]+(b)[2];}
#define VectorCopy(a,b) {(b)[0]=(a)[0];(b)[1]=(a)[1];(b)[2]=(a)[2];}
#define VectorScale(a,b,c) {(c)[0]=(b)*(a)[0];(c)[1]=(b)*(a)[1];(c)[2]=(b)*(a)[2];}