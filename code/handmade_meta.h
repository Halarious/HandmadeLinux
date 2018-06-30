
typedef enum
  {
    MetaType_s32,
    MetaType_u32,                         	 
    MetaType_bool32,                      	 
    MetaType_r32,                         	 
    MetaType_v2,                                    
    MetaType_v3,
    MetaType_rectangle2,
    MetaType_rectangle3,
    MetaType_world,
    MetaType_world_chunk,
    MetaType_world_position,
    MetaType_entity_type,                 	 
    MetaType_sim_region,
    MetaType_sim_entity,
    MetaType_sim_entity_hash,
    MetaType_sim_entity_collision_volume_group,
    MetaType_hit_point,                   	 
    MetaType_entity_reference,            	 
  } meta_type;

typedef enum
  {
    MetaMemberFlag_IsPointer = 0x1,
  } member_definition_flag;

typedef struct
{
  u32 Flags;
  meta_type Type;
  char* Name;
  u32 Offset;
} member_definition;

