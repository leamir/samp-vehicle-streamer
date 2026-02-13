#if defined DEBUG_MODE && !defined DEBUG_DYNAMIC_VEHICLES
    #define SHOULD_UNDEF_DEBUG_DYNAMIC_VEHICLES
    #define DEBUG_DYNAMIC_VEHICLES
#endif

#if !defined _PawnPlus_included
    #error Please include PawnPlus before this module.
#endif
#if !defined _streamer_included
    #error Please include Streamer before this module.
#endif

#if !defined DYNAMIC_VEHICLES_SPAWN_QUEUE_PROCESS_INTERVAL
#define DYNAMIC_VEHICLES_SPAWN_QUEUE_PROCESS_INTERVAL 250 // in milliseconds
// The spawn queue spawns vehicles in order
// Each run it will create as many vehicles as possible.
// If the vehicle wasn't created due to reaching MAX_VEHICLES, the queue stops processing until the next interval.
// The vehicle that coudn't be created remains at the front of the queue.
#endif

#if !defined DYNAMIC_VEHICLES_DESPAWN_CHECK_INTERVAL
#define DYNAMIC_VEHICLES_DESPAWN_CHECK_INTERVAL 5000 // in milliseconds
// Every interval, checks for vehicles that need to be despawned.
// A vehicle needs to be despawned when there are no players in its area;
// Vehicles need to have no players around for at least DYNAMIC_VEHICLES_DESPAWN_CHECK_INTERVAL milliseconds before being despawned.
#endif

#if !defined STREAMER_EXTRA_ID_DYNAMIC_VEHICLE
#define STREAMER_EXTRA_ID_DYNAMIC_VEHICLE E_STREAMER_CUSTOM(1)
// Just make sure you're not using this extra ID for something else in your gamemode.
// It's used to link a dynamic vehicle to its area.
#endif

// #define VEHICLE_STREAMER_USE_ATTACHED_OBJECTS
// This isn't enabled by default because it probably adds some overhead to the include, which some ppl might not want.
// I haven't really tested whether there's noticiable overhead or not, but don't want to risk it (there probably is)


////////////////////////////////////////////////////////////////////////////////////////////
// PLEASE DO NOT MODIFY BELOW // PLEASE DO NOT MODIFY BELOW // PLEASE DO NOT MODIFY BELOW //
////////////////////////////////////////////////////////////////////////////////////////////

#if defined VEHICLE_STREAMER_USE_ATTACHED_OBJECTS
    #if !defined HideObjectForPlayer || !defined ShowObjectForPlayer
        #error Attaching Objects to Dynamic Vehicle requires either YSF or open.mp v1.1.0.2612 or higher (Support for [Hide/Show]ObjectForPlayer)
    #endif


static enum e_ATTACHED_OBJECT_DATA
{
    attachedObj_ObjectId,
    attachedObj_OffsetX,
    attachedObj_OffsetY,
    attachedObj_OffsetZ,
    attachedObj_RotationX,
    attachedObj_RotationY,
    attachedObj_RotationZ
}
#endif


#if !defined __TAG
    #if defined NO_TAGS
        #define DynamicVehicle: _:
        #define __TAG(%0) _
    #elseif defined STRONG_TAGS
        #define __TAG(%0) T_%0
    #else
        #define __TAG(%0) t_%0
        #if !defined WEAK_TAGS
	        #define WEAK_TAGS
        #endif
    #endif
#endif

#define INVALID_DYNAMIC_VEHICLE_ID DynamicVehicle:-1

static enum e_DYNAMIC_VEHICLE_DATA
{
    bool:dynVeh_IsSpawned,
    dynVeh_internalVehicleId, // The actual vehicle ID when spawned
    STREAMER_TAG_AREA:dynVeh_areaId,
    dynVeh_playerCount,
    dynVeh_despawnScheduledTime,

    dynVeh_ModelId,
    dynVeh_Colour1,
    dynVeh_Colour2,
    Float:dynVeh_SpawnX,
    Float:dynVeh_SpawnY,
    Float:dynVeh_SpawnZ,
    Float:dynVeh_SpawnAngle,
    dynVeh_RespawnDelay,
    bool:dynVeh_AddSiren,

    dynVeh_numberPlate[33],

    dynVeh_PaintJob,

    dynVeh_VirtualWorld,
    dynVeh_InteriorId,

    Float:dynVeh_CurrentHealth,
    Float:dynVeh_CurrentX,
    Float:dynVeh_CurrentY,
    Float:dynVeh_CurrentZ,

    Float:dynVeh_CurrentVelocityX,
    Float:dynVeh_CurrentVelocityY,
    Float:dynVeh_CurrentVelocityZ,

    dynVeh_doorFrontLeft,
    dynVeh_doorFrontRight,
    dynVeh_doorRearLeft,
    dynVeh_doorRearRight,

    dynVeh_windowFrontLeft,
    dynVeh_windowFrontRight,
    dynVeh_windowRearLeft,
    dynVeh_windowRearRight,

    __TAG(VEHICLE_PANEL_STATUS):dynVeh_panelState,
    __TAG(VEHICLE_DOOR_STATUS):dynVeh_doorsState,
    __TAG(VEHICLE_LIGHT_STATUS):dynVeh_lightsState,
    __TAG(VEHICLE_TYRE_STATUS):dynVeh_tyresState,

    dynVeh_engine,
    dynVeh_lights,
    dynVeh_alarm,
    dynVeh_doors,
    dynVeh_bonnet,
    dynVeh_boot,
    dynVeh_objective,

    Float:dynVeh_CurrentAngle,

    dynVeh_SirenState,

    dynVeh_respawnTimerId,

    Map:dynVeh_playerVehicleParams,

    Map:dynVeh_vehicleComponents,

    #if defined VEHICLE_STREAMER_USE_ATTACHED_OBJECTS
    List:dynVeh_attachedObjects,
    List:dynVeh_attachedDynamicObjects,
    #endif

    #if defined GetVehicleRespawnTick || defined SetVehicleRespawnTick
    dynVeh_respawnTick,
    #endif
    #if defined GetVehicleOccupiedTick || defined SetVehicleOccupiedTick
    dynVeh_lastOccupiedTick,
    #endif

    #if defined GetVehicleLastDriver || defined HasVehicleBeenOccupied
    dynVeh_lastDriverId,
    #endif

    #if defined DEBUG_DYNAMIC_VEHICLES
    STREAMER_TAG_3D_TEXT_LABEL:debug_3dTextId,
    #endif

    dynVeh_int_largestTag // keep at last, used to calculate array size
};

new 
    Pool:g_DynamicVehicles;
static 
    DynamicVehicle:s_VehicleToDynamic[MAX_VEHICLES],
    List:s_DynamicVehiclesToSpawn,
    List:s_DynamicVehiclesDespawnScheduled;

#if defined VEHICLE_STREAMER_USE_ATTACHED_OBJECTS
static 
    STREAMER_TAG_AREA:s_dynObjHiddenAreaId;
    // This area is used so dynamic objects attached to streamed-out vehicles don't render.
    // The object is just set to only render if player is in this area (which should never happen - it's outside the playable area)
#endif

#if defined VEHICLE_STREAMER_USE_ATTACHED_OBJECTS
    #if defined foreach
        #define vehicle_streamer_loop_all_players(%0) foreach (%0 : Player)
    #else
        #define vehicle_streamer_loop_all_players(new%0) for (new %0=0; %0 < MAX_PLAYERS; %0++) if (IsPlayerConnected(%0)) 
        #define vehicle_streamer_loop_all_players(%0) for (%0=0; %0 < MAX_PLAYERS; %0++) if (IsPlayerConnected(%0)) 
    #endif
#endif

static int_VS_IsValidVehicleModel(modelid)
{
    return (modelid >= 400 && modelid <= 611);
}

stock IsValidDynamicVehicle(DynamicVehicle:dynamicVehicleId)
{
    return (dynamicVehicleId >= DynamicVehicle:0 && pool_has(g_DynamicVehicles, _:dynamicVehicleId));
}

stock GetDynamicVehicleData(DynamicVehicle:dynamicVehicleId, array[])
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    pool_get_arr(g_DynamicVehicles, _:dynamicVehicleId, array, _:dynVeh_int_largestTag);
    return 1;
}

stock DynamicVehicle:GetDynamicVehicleFromInternalVehicleId(internalVehicleId)
{
    if (internalVehicleId < 0 || internalVehicleId >= MAX_VEHICLES)
        return INVALID_DYNAMIC_VEHICLE_ID;

    if (IsValidDynamicVehicle(s_VehicleToDynamic[internalVehicleId]))
        return s_VehicleToDynamic[internalVehicleId];

    return INVALID_DYNAMIC_VEHICLE_ID;
}

stock UpdateDynamicVehicleData(DynamicVehicle:dynamicVehicleId, const array[])
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    pool_set_arr(g_DynamicVehicles, _:dynamicVehicleId, array, _:dynVeh_int_largestTag);
    return 1;
}

static int_clearDynamicVehicleData(array[])
{
    array[dynVeh_IsSpawned] = false;
    array[dynVeh_internalVehicleId] = INVALID_VEHICLE_ID;
    array[dynVeh_areaId] = STREAMER_TAG_AREA:INVALID_STREAMER_ID;
    array[dynVeh_playerCount] = 0;
    array[dynVeh_despawnScheduledTime] = 0;

    array[dynVeh_ModelId] = 0;
    array[dynVeh_Colour1] = 0;
    array[dynVeh_Colour2] = 0;
    array[dynVeh_SpawnX] = 0.0;
    array[dynVeh_SpawnY] = 0.0;
    array[dynVeh_SpawnZ] = 0.0;
    array[dynVeh_SpawnAngle] = 0.0;
    array[dynVeh_RespawnDelay] = -1;
    array[dynVeh_AddSiren] = false;

    array[dynVeh_CurrentHealth] = 1000.0;
    array[dynVeh_CurrentX] = array[dynVeh_SpawnX];
    array[dynVeh_CurrentY] = array[dynVeh_SpawnY];
    array[dynVeh_CurrentZ] = array[dynVeh_SpawnZ];

    array[dynVeh_CurrentVelocityX] = 0.0;
    array[dynVeh_CurrentVelocityY] = 0.0;
    array[dynVeh_CurrentVelocityZ] = 0.0;

    array[dynVeh_VirtualWorld] = 0;
    array[dynVeh_InteriorId] = 0;

    array[dynVeh_PaintJob] = -1;

    format(array[dynVeh_numberPlate], 33, "");

    array[dynVeh_doorFrontLeft] = 0;
    array[dynVeh_doorFrontRight] = 0;
    array[dynVeh_doorRearLeft] = 0;
    array[dynVeh_doorRearRight] = 0;

    array[dynVeh_windowFrontLeft] = 1;
    array[dynVeh_windowFrontRight] = 1;
    array[dynVeh_windowRearLeft] = 1;
    array[dynVeh_windowRearRight] = 1;

    array[dynVeh_panelState] = __TAG(VEHICLE_PANEL_STATUS):0;
    array[dynVeh_doorsState] = __TAG(VEHICLE_DOOR_STATUS):0;
    array[dynVeh_lightsState] = __TAG(VEHICLE_LIGHT_STATUS):0;
    array[dynVeh_tyresState] = __TAG(VEHICLE_TYRE_STATUS):0;

    array[dynVeh_engine] = 0;
    array[dynVeh_lights] = 0;
    array[dynVeh_alarm] = 0;
    array[dynVeh_doors] = 0;
    array[dynVeh_bonnet] = 0;
    array[dynVeh_boot] = 0;

    array[dynVeh_objective] = 0;

    array[dynVeh_CurrentAngle] = 0.0;

    array[dynVeh_SirenState] = 0;

    array[dynVeh_respawnTimerId] = -1;

    if (map_valid(array[dynVeh_playerVehicleParams]))
        map_clear_deep(array[dynVeh_playerVehicleParams]);
    else
        array[dynVeh_playerVehicleParams] = map_new();

    if (map_valid(array[dynVeh_vehicleComponents]))
        map_clear_deep(array[dynVeh_vehicleComponents]);
    else
        array[dynVeh_vehicleComponents] = map_new();

    #if defined VEHICLE_STREAMER_USE_ATTACHED_OBJECTS
    if (list_valid(array[dynVeh_attachedObjects]))
    {
        for(new Iter:i=list_iter(array[dynVeh_attachedObjects]);iter_inside(i);iter_move_next(i))
        {
            new objId = iter_get(i);
            if (IsValidObject(objId))
                DestroyObject(objId);
        }

        list_clear_deep(array[dynVeh_attachedObjects]);
    }
    else
        array[dynVeh_attachedObjects] = list_new();

    if (list_valid(array[dynVeh_attachedDynamicObjects]))
    {
        for(new Iter:i=list_iter(array[dynVeh_attachedDynamicObjects]);iter_inside(i);iter_move_next(i))
        {
            new STREAMER_TAG_OBJECT:objId = iter_get(i);
            if (IsValidDynamicObject(objId))
                DestroyDynamicObject(objId);
        }

        list_clear_deep(array[dynVeh_attachedDynamicObjects]);
    }
    else
        array[dynVeh_attachedDynamicObjects] = list_new();
    #endif

    #if defined GetVehicleRespawnTick || defined SetVehicleRespawnTick
    array[dynVeh_respawnTick] = 0;
    #endif
    #if defined GetVehicleOccupiedTick || defined SetVehicleOccupiedTick
    array[dynVeh_lastOccupiedTick] = 0;
    #endif

    #if defined GetVehicleLastDriver || defined HasVehicleBeenOccupied
    array[dynVeh_lastDriverId] = INVALID_PLAYER_ID;
    #endif

    #if defined DEBUG_DYNAMIC_VEHICLES
    array[debug_3dTextId] = STREAMER_TAG_3D_TEXT_LABEL:INVALID_STREAMER_ID;
    #endif

    return 1;
}
 
stock DynamicVehicle:GetDynamicVehicleForAreaId(STREAMER_TAG_AREA:areaId)
{
    if (areaId == STREAMER_TAG_AREA:INVALID_STREAMER_ID)
        return INVALID_DYNAMIC_VEHICLE_ID;

    new 
        DynamicVehicle:dynamicVehicleId = DynamicVehicle:Streamer_GetIntData(STREAMER_TYPE_AREA, areaId, STREAMER_EXTRA_ID_DYNAMIC_VEHICLE);

    if (IsValidDynamicVehicle(dynamicVehicleId))
        return dynamicVehicleId;

    return INVALID_DYNAMIC_VEHICLE_ID;
}

static stock int_VS_IsATrain(modelid)
{
    return (modelid == 449 || modelid == 537 || modelid == 538 || modelid == 569 || modelid == 570 || modelid == 590);
}

/* 
Functions
*/
stock DynamicVehicle:CreateDynamicVehicle(modelid, Float:x, Float:y, Float:z, Float:angle, colour1 = -1, colour2 = -1, respawnDelay = -1, bool:addSiren = false, Float:streamRadius = 300.0)
{
    if (!int_VS_IsValidVehicleModel(modelid))
        return INVALID_DYNAMIC_VEHICLE_ID;

    if (int_VS_IsATrain(modelid))
        return INVALID_DYNAMIC_VEHICLE_ID; // Trains are not supported

    new
        array[e_DYNAMIC_VEHICLE_DATA];

    int_clearDynamicVehicleData(array);

    if (colour1 == -1)
        colour1 = random(255);
    if (colour2 == -1)
        colour2 = random(255);

    array[dynVeh_ModelId] = modelid;
    array[dynVeh_Colour1] = colour1;
    array[dynVeh_Colour2] = colour2;
    array[dynVeh_SpawnX] = x;
    array[dynVeh_SpawnY] = y;
    array[dynVeh_SpawnZ] = z;
    array[dynVeh_CurrentX] = array[dynVeh_SpawnX];
    array[dynVeh_CurrentY] = array[dynVeh_SpawnY];
    array[dynVeh_CurrentZ] = array[dynVeh_SpawnZ];
    array[dynVeh_SpawnAngle] = angle;
    array[dynVeh_RespawnDelay] = respawnDelay;
    array[dynVeh_AddSiren] = addSiren;

    new 
        STREAMER_TAG_AREA:areaId = CreateDynamicSphere(x, y, z, streamRadius, 0, 0);

    if (areaId == STREAMER_TAG_AREA:INVALID_STREAMER_ID)
        return INVALID_DYNAMIC_VEHICLE_ID;

    array[dynVeh_areaId] = areaId;

    #if defined DEBUG_DYNAMIC_VEHICLES
        new 
            STREAMER_TAG_3D_TEXT_LABEL:debug3dTextId = CreateDynamic3DTextLabel("Loading...", 0xFFFFFFFF, x, y, z + 1.0, .drawdistance = streamRadius+10.0, .streamdistance = streamRadius+10.0, .testlos = 1);
        array[debug_3dTextId] = debug3dTextId;
    #endif

    new 
        DynamicVehicle:dynamicVehicleId = DynamicVehicle:pool_add_arr(g_DynamicVehicles, array);

    Streamer_SetIntData(STREAMER_TYPE_AREA, areaId, STREAMER_EXTRA_ID_DYNAMIC_VEHICLE, _:dynamicVehicleId);

    #if defined DEBUG_DYNAMIC_VEHICLES
        UpdateDynamicVehicleDebugText(dynamicVehicleId);
    #endif

    return dynamicVehicleId;
}

stock DestroyDynamicVehicle(DynamicVehicle:dynamicVehicleId)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new
        dynVehData[e_DYNAMIC_VEHICLE_DATA];
    
    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
    {
        DestroyVehicle(dynVehData[dynVeh_internalVehicleId]);
        s_VehicleToDynamic[dynVehData[dynVeh_internalVehicleId]] = INVALID_DYNAMIC_VEHICLE_ID;
    }

    #if defined DEBUG_DYNAMIC_VEHICLES
        if (dynVehData[debug_3dTextId] != STREAMER_TAG_3D_TEXT_LABEL:INVALID_STREAMER_ID)
            DestroyDynamic3DTextLabel(dynVehData[debug_3dTextId]);
    #endif

    if ((new tmpid = list_find(s_DynamicVehiclesDespawnScheduled, _:dynamicVehicleId)) != -1)
        list_remove(s_DynamicVehiclesDespawnScheduled, tmpid);

    if ((tmpid = list_find(s_DynamicVehiclesToSpawn, _:dynamicVehicleId)) != -1)
        list_remove(s_DynamicVehiclesToSpawn, tmpid);

    DestroyDynamicArea(dynVehData[dynVeh_areaId]);
    pool_remove(g_DynamicVehicles, _:dynamicVehicleId);

    return 1;
}

#if defined DEBUG_DYNAMIC_VEHICLES
static GetDynVehicleStateToText(DynamicVehicle:dynamicVehicleId, result[], maxlen = sizeof(result))
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
    {
        format(result, maxlen, "INVALID");
        return 0;
    }

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        format(result, maxlen, "SPAWNED");
    else
        format(result, maxlen, "NOT SPAWNED");

    if (list_find(s_DynamicVehiclesDespawnScheduled, _:dynamicVehicleId) != -1)
        strcat(result, " (DESPAWN SCHEDULED)", maxlen);

    if (list_find(s_DynamicVehiclesToSpawn, _:dynamicVehicleId) != -1)
        strcat(result, " (IN SPAWN QUEUE)", maxlen);
    return 1;
}

static UpdateDynamicVehicleDebugText(DynamicVehicle:dynamicVehicleId)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    new 
        STREAMER_TAG_3D_TEXT_LABEL:debug3dTextId = dynVehData[debug_3dTextId];

    if (debug3dTextId == STREAMER_TAG_3D_TEXT_LABEL:INVALID_STREAMER_ID)
        return 0;

    new 
        text[128], 
        stateText[64];
    GetDynVehicleStateToText(dynamicVehicleId, stateText, sizeof(stateText));

    format(text, sizeof(text),
        "DynVeh Id: %d - State: %s\nArea Id: %d - Players: %d",
        _:dynamicVehicleId, stateText,
        _:dynVehData[dynVeh_areaId], dynVehData[dynVeh_playerCount]
    );


    if (dynVehData[dynVeh_IsSpawned])
    {
        Streamer_SetIntData(STREAMER_TYPE_3D_TEXT_LABEL, debug3dTextId, E_STREAMER_ATTACHED_VEHICLE, dynVehData[dynVeh_internalVehicleId]);
        Streamer_SetFloatData(STREAMER_TYPE_3D_TEXT_LABEL, debug3dTextId, E_STREAMER_X, 0.0);
        Streamer_SetFloatData(STREAMER_TYPE_3D_TEXT_LABEL, debug3dTextId, E_STREAMER_Y, 0.0);
        Streamer_SetFloatData(STREAMER_TYPE_3D_TEXT_LABEL, debug3dTextId, E_STREAMER_Z, 0.0);
        Streamer_SetFloatData(STREAMER_TYPE_3D_TEXT_LABEL, debug3dTextId, E_STREAMER_ATTACH_OFFSET_Z, 2.0);
    }
    else
    {
        Streamer_SetIntData(STREAMER_TYPE_3D_TEXT_LABEL, debug3dTextId, E_STREAMER_ATTACHED_VEHICLE, INVALID_VEHICLE_ID);
        Streamer_SetFloatData(STREAMER_TYPE_3D_TEXT_LABEL, debug3dTextId, E_STREAMER_X, dynVehData[dynVeh_CurrentX]);
        Streamer_SetFloatData(STREAMER_TYPE_3D_TEXT_LABEL, debug3dTextId, E_STREAMER_Y, dynVehData[dynVeh_CurrentY]);
        Streamer_SetFloatData(STREAMER_TYPE_3D_TEXT_LABEL, debug3dTextId, E_STREAMER_Z, dynVehData[dynVeh_CurrentZ]);
    }

    Streamer_SetIntData(STREAMER_TYPE_3D_TEXT_LABEL, debug3dTextId, E_STREAMER_WORLD_ID, dynVehData[dynVeh_VirtualWorld]);
    Streamer_SetIntData(STREAMER_TYPE_3D_TEXT_LABEL, debug3dTextId, E_STREAMER_INTERIOR_ID, dynVehData[dynVeh_InteriorId]);

    UpdateDynamic3DTextLabelText(debug3dTextId, 0xFFFFFFFF, text);

    return 1;
}
#endif

static AddDynamicVehicleToSpawnQueue(DynamicVehicle:dynamicVehicleId)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    // Remove from despawn schedule if there
    new despawnIndex = list_find(s_DynamicVehiclesDespawnScheduled, _:dynamicVehicleId);
    if (despawnIndex != -1)
    {
        dynVehData[dynVeh_despawnScheduledTime] = 0;
        list_remove(s_DynamicVehiclesDespawnScheduled, despawnIndex);
    }

    if (dynVehData[dynVeh_IsSpawned])
        return 0;

    list_add(s_DynamicVehiclesToSpawn, _:dynamicVehicleId);

    return 1;
}

forward int_VS_ProcessDynVehSpawnQueue();
public int_VS_ProcessDynVehSpawnQueue()
{
    while (list_size(s_DynamicVehiclesToSpawn) > 0)
    {
        new 
            DynamicVehicle:dynamicVehicleId = DynamicVehicle:list_get(s_DynamicVehiclesToSpawn, 0);

        if (!IsValidDynamicVehicle(dynamicVehicleId))
        {
            list_remove(s_DynamicVehiclesToSpawn, 0);
            continue;
        }
        
        new 
            dynVehData[e_DYNAMIC_VEHICLE_DATA];

        GetDynamicVehicleData(dynamicVehicleId, dynVehData);

        if (dynVehData[dynVeh_IsSpawned])
        {
            list_remove(s_DynamicVehiclesToSpawn, 0);
            continue;
        }

        new 
            vehicleId = CreateVehicle(
                dynVehData[dynVeh_ModelId],
                dynVehData[dynVeh_CurrentX],
                dynVehData[dynVeh_CurrentY],
                dynVehData[dynVeh_CurrentZ],
                dynVehData[dynVeh_SpawnAngle],
                dynVehData[dynVeh_Colour1],
                dynVehData[dynVeh_Colour2],
                -1, // Respawn delay should be handled manually due to vehicle creation/destruction
                dynVehData[dynVeh_AddSiren]
            );
        
        if (vehicleId == INVALID_VEHICLE_ID)
            break; // Could not create vehicle, probably reached MAX_VEHICLES

        // Streamer_SetIntData(STREAMER_TYPE_AREA, dynVehData[dynVeh_areaId], E_STREAMER_ATTACHED_VEHICLE, vehicleId);
        // Streamer_SetFloatData(STREAMER_TYPE_AREA, dynVehData[dynVeh_areaId], E_STREAMER_X, 0.0);
        // Streamer_SetFloatData(STREAMER_TYPE_AREA, dynVehData[dynVeh_areaId], E_STREAMER_Y, 0.0);
        // Streamer_SetFloatData(STREAMER_TYPE_AREA, dynVehData[dynVeh_areaId], E_STREAMER_Z, 0.0);
        AttachDynamicAreaToVehicle(dynVehData[dynVeh_areaId], vehicleId);
        // Streamer_SetIntData(STREAMER_TYPE_AREA, dynVehData[dynVeh_areaId], E_STREAMER_WORLD_ID, dynVehData[dynVeh_VirtualWorld]);
        // Streamer_SetIntData(STREAMER_TYPE_AREA, dynVehData[dynVeh_areaId], E_STREAMER_INTERIOR_ID, dynVehData[dynVeh_InteriorId]);

        dynVehData[dynVeh_IsSpawned] = true;
        dynVehData[dynVeh_internalVehicleId] = vehicleId;

        s_VehicleToDynamic[vehicleId] = dynamicVehicleId;

        UpdateDynamicVehicleData(dynamicVehicleId, dynVehData);

        #if defined VEHICLE_STREAMER_USE_ATTACHED_OBJECTS

        // Re-attach any previously-attached objects
        new bool:isFirstPass = true;
        vehicle_streamer_loop_all_players(new playerid) {
            for(new Iter:i=list_iter(dynVehData[dynVeh_attachedObjects]);iter_inside(i);iter_move_next(i))
            {
                new 
                    objData[e_ATTACHED_OBJECT_DATA];
                iter_get_arr(i, objData, 7);
                ShowObjectForPlayer(playerid, objData[attachedObj_ObjectId]);
        
                if (isFirstPass)
                {
                    AttachObjectToVehicle(
                        objData[attachedObj_ObjectId], vehicleId,
                        objData[attachedObj_OffsetX], objData[attachedObj_OffsetY], objData[attachedObj_OffsetZ],
                        objData[attachedObj_RotationX], objData[attachedObj_RotationY], objData[attachedObj_RotationZ]
                    );
                }
            }

            isFirstPass = false;
        }

        for(new Iter:i=list_iter(dynVehData[dynVeh_attachedDynamicObjects]);iter_inside(i);iter_move_next(i))
        {
            new 
                objData[e_ATTACHED_OBJECT_DATA];
            iter_get_arr(i, objData, 7);

            AttachDynamicObjectToVehicle(
                STREAMER_TAG_OBJECT:objData[attachedObj_ObjectId], vehicleId,
                objData[attachedObj_OffsetX], objData[attachedObj_OffsetY], objData[attachedObj_OffsetZ],
                objData[attachedObj_RotationX], objData[attachedObj_RotationY], objData[attachedObj_RotationZ]
            );
            Streamer_SetIntData(STREAMER_TYPE_OBJECT, objData[attachedObj_ObjectId], STREAMER_TYPE_AREA, INVALID_STREAMER_ID);
        }
        #endif

        // Set vehicle damage states
        SetVehicleHealth(vehicleId, dynVehData[dynVeh_CurrentHealth]);
        SetVehicleVelocity(vehicleId,
            dynVehData[dynVeh_CurrentVelocityX],
            dynVehData[dynVeh_CurrentVelocityY],
            dynVehData[dynVeh_CurrentVelocityZ]
        );
        SetVehicleParamsCarDoors(vehicleId,
            dynVehData[dynVeh_doorFrontLeft],
            dynVehData[dynVeh_doorFrontRight],
            dynVehData[dynVeh_doorRearLeft],
            dynVehData[dynVeh_doorRearRight]
        );
        SetVehicleParamsCarWindows(vehicleId,
            dynVehData[dynVeh_windowFrontLeft],
            dynVehData[dynVeh_windowFrontRight],
            dynVehData[dynVeh_windowRearLeft],
            dynVehData[dynVeh_windowRearRight]
        );
        UpdateVehicleDamageStatus(
            vehicleId,
            dynVehData[dynVeh_panelState],
            dynVehData[dynVeh_doorsState],
            dynVehData[dynVeh_lightsState],
            dynVehData[dynVeh_tyresState]
        );

        SetVehicleParamsSirenState(vehicleId, bool:dynVehData[dynVeh_SirenState]);

        SetVehicleVirtualWorld(vehicleId, dynVehData[dynVeh_VirtualWorld]);
        LinkVehicleToInterior(vehicleId, dynVehData[dynVeh_InteriorId]);

        SetVehicleNumberPlate(vehicleId, dynVehData[dynVeh_numberPlate]);

        if (dynVehData[dynVeh_PaintJob] != -1)
            ChangeVehiclePaintjob(vehicleId, dynVehData[dynVeh_PaintJob]);

        list_remove(s_DynamicVehiclesToSpawn, 0);

        // The normal samp limitations for SetVehicleParamsForPlayer also apply, but if player is in vehicle stream area, the param is applied and works
        // This causes some inconsistencies based on the normal samp param rules, but who cares, you should re-apply on stream in anyway
        for(new Iter:i = map_iter(dynVehData[dynVeh_playerVehicleParams]); iter_inside(i); iter_move_next(i)) // for_map(i : dynVehData[dynVeh_playerVehicleParams])
        {
            new playerid, data[2]; 
            iter_get_key_safe(i, playerid);
            iter_get_value_arr_safe(i, data);

            SetVehicleParamsForPlayer(vehicleId, playerid, data[0], data[1]);
        }

        for(new Iter:i = map_iter(dynVehData[dynVeh_vehicleComponents]); iter_inside(i); iter_move_next(i)) // for_map(i : dynVehData[dynVeh_vehicleComponents])
        {
            new 
                componentId;
            iter_get_value_safe(i, componentId);

            AddVehicleComponent(vehicleId, componentId);
        }

        #if defined DEBUG_DYNAMIC_VEHICLES
            UpdateDynamicVehicleDebugText(dynamicVehicleId);
        #endif

        if(pawn_public_exists("OnDynamicVehicleStreamIn"))
            pawn_call_public("OnDynamicVehicleStreamIn", "iii", _:dynamicVehicleId, INVALID_PLAYER_ID, 1);

        wait_ms(0); // Yield to prevent blocking
    }

    return 1;
}

forward int_VS_ProcessDynVehDespawns();
public int_VS_ProcessDynVehDespawns()
{
    new currentTime = GetTickCount();

    for_list(i : s_DynamicVehiclesDespawnScheduled)
    {
        new DynamicVehicle:dynamicVehicleId = DynamicVehicle:iter_get(i);

        if (!IsValidDynamicVehicle(dynamicVehicleId))
        {
            iter_erase(i, .stay = true);
            continue;
        }

        new 
            dynVehData[e_DYNAMIC_VEHICLE_DATA];
        GetDynamicVehicleData(dynamicVehicleId, dynVehData);
        
        if (dynVehData[dynVeh_playerCount] > 0)
        {
            iter_erase(i, .stay = true);
            continue;
        }

        new scheduledTime = dynVehData[dynVeh_despawnScheduledTime];
        if (scheduledTime == 0 || (currentTime - scheduledTime) < DYNAMIC_VEHICLES_DESPAWN_CHECK_INTERVAL/1000)
            continue;

        //Save all vehicle data in the vars so we can restore it later
        GetVehicleHealth(dynVehData[dynVeh_internalVehicleId], dynVehData[dynVeh_CurrentHealth]);
        GetVehicleParamsCarDoors(dynVehData[dynVeh_internalVehicleId],
            dynVehData[dynVeh_doorFrontLeft],
            dynVehData[dynVeh_doorFrontRight],
            dynVehData[dynVeh_doorRearLeft],
            dynVehData[dynVeh_doorRearRight]
        );
        GetVehicleParamsCarWindows(dynVehData[dynVeh_internalVehicleId],
            dynVehData[dynVeh_windowFrontLeft],
            dynVehData[dynVeh_windowFrontRight],
            dynVehData[dynVeh_windowRearLeft],
            dynVehData[dynVeh_windowRearRight]
        );
        GetVehicleDamageStatus(
            dynVehData[dynVeh_internalVehicleId],
            dynVehData[dynVeh_panelState],
            dynVehData[dynVeh_doorsState],
            dynVehData[dynVeh_lightsState],
            dynVehData[dynVeh_tyresState]
        );
        GetVehicleParamsEx(
            dynVehData[dynVeh_internalVehicleId],
            dynVehData[dynVeh_engine],
            dynVehData[dynVeh_lights],
            dynVehData[dynVeh_alarm],
            dynVehData[dynVeh_doors],
            dynVehData[dynVeh_bonnet],
            dynVehData[dynVeh_boot],
            dynVehData[dynVeh_objective]
        );
        GetVehiclePos(dynVehData[dynVeh_internalVehicleId], dynVehData[dynVeh_CurrentX], dynVehData[dynVeh_CurrentY], dynVehData[dynVeh_CurrentZ]);
        GetVehicleZAngle(dynVehData[dynVeh_internalVehicleId], dynVehData[dynVeh_CurrentAngle]);
        dynVehData[dynVeh_SirenState] = GetVehicleParamsSirenState(dynVehData[dynVeh_internalVehicleId]);
        GetVehicleVelocity(dynVehData[dynVeh_internalVehicleId], dynVehData[dynVeh_CurrentVelocityX], dynVehData[dynVeh_CurrentVelocityY], dynVehData[dynVeh_CurrentVelocityZ]);

        Streamer_SetIntData(STREAMER_TYPE_AREA, dynVehData[dynVeh_areaId], E_STREAMER_ATTACHED_VEHICLE, INVALID_VEHICLE_ID);
        Streamer_SetFloatData(STREAMER_TYPE_AREA, dynVehData[dynVeh_areaId], E_STREAMER_X, dynVehData[dynVeh_CurrentX]);
        Streamer_SetFloatData(STREAMER_TYPE_AREA, dynVehData[dynVeh_areaId], E_STREAMER_Y, dynVehData[dynVeh_CurrentY]);
        Streamer_SetFloatData(STREAMER_TYPE_AREA, dynVehData[dynVeh_areaId], E_STREAMER_Z, dynVehData[dynVeh_CurrentZ]);

        dynVehData[dynVeh_IsSpawned] = false;

        #if defined VEHICLE_STREAMER_USE_ATTACHED_OBJECTS

        // Hide any previously-attached objects
        vehicle_streamer_loop_all_players(new playerid) {
            for(new Iter:x=list_iter(dynVehData[dynVeh_attachedObjects]);iter_inside(x);iter_move_next(x))
            {
                new 
                    objData[e_ATTACHED_OBJECT_DATA];
                iter_get_arr(x, objData, 7);
                HideObjectForPlayer(playerid, objData[attachedObj_ObjectId]);
            }
        }

        for(new Iter:x=list_iter(dynVehData[dynVeh_attachedDynamicObjects]);iter_inside(x);iter_move_next(x))
        {
            new 
                objData[e_ATTACHED_OBJECT_DATA];
            iter_get_arr(x, objData, 7);

            Streamer_SetIntData(STREAMER_TYPE_OBJECT, objData[attachedObj_ObjectId], E_STREAMER_AREA_ID, _:s_dynObjHiddenAreaId);
            Streamer_SetIntData(STREAMER_TYPE_OBJECT, objData[attachedObj_ObjectId], E_STREAMER_ATTACHED_VEHICLE, _:INVALID_STREAMER_ID);
        }
        #endif

        DestroyVehicle(dynVehData[dynVeh_internalVehicleId]);
        s_VehicleToDynamic[dynVehData[dynVeh_internalVehicleId]] = INVALID_DYNAMIC_VEHICLE_ID;
        dynVehData[dynVeh_internalVehicleId] = INVALID_VEHICLE_ID;

        UpdateDynamicVehicleData(dynamicVehicleId, dynVehData);

        iter_erase(i, .stay = true);
        
        #if defined DEBUG_DYNAMIC_VEHICLES
            UpdateDynamicVehicleDebugText(dynamicVehicleId);
        #endif

        if(pawn_public_exists("OnDynamicVehicleStreamOut"))
            pawn_call_public("OnDynamicVehicleStreamOut", "iii", _:dynamicVehicleId, INVALID_PLAYER_ID, 1);
    }

    return 1;
}

// Setup

public OnGameModeInit() {
    g_DynamicVehicles = pool_new();
    s_DynamicVehiclesToSpawn = list_new();
    s_DynamicVehiclesDespawnScheduled = list_new();

    #if defined VEHICLE_STREAMER_USE_ATTACHED_OBJECTS
    s_dynObjHiddenAreaId = CreateDynamicSphere(5000.0, 5000.0, 0.0, 0.1, .worldid = 65535, .interiorid = 32767, .priority = -10000);
    #endif

    SetTimer("int_VS_ProcessDynVehDespawns", DYNAMIC_VEHICLES_DESPAWN_CHECK_INTERVAL, true);
    SetTimer("int_VS_ProcessDynVehSpawnQueue", DYNAMIC_VEHICLES_SPAWN_QUEUE_PROCESS_INTERVAL, true);

    #if defined ALS_VS_OnGameModeInit
        return ALS_VS_OnGameModeInit();
    #else
        return 1;
    #endif
}
#if defined _ALS_OnGameModeInit
    #undef OnGameModeInit
#else
    #define _ALS_OnGameModeInit
#endif
#define OnGameModeInit ALS_VS_OnGameModeInit
#if defined ALS_VS_OnGameModeInit
    forward ALS_VS_OnGameModeInit();
#endif

public OnPlayerEnterDynamicArea(playerid, STREAMER_TAG_AREA:areaid) {
    new DynamicVehicle:dynamicVehicleId = DynamicVehicle:GetDynamicVehicleForAreaId(areaid);
    if (!IsValidDynamicVehicle(dynamicVehicleId))
    {
        #if defined ALS_VS_OnPlayerEnterDynamicArea
            return ALS_VS_OnPlayerEnterDynamicArea(playerid, STREAMER_TAG_AREA:areaid);
        #else
            return 1;
        #endif
    }
    
    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    dynVehData[dynVeh_playerCount]++;

    if (dynVehData[dynVeh_playerCount] == 1)
        AddDynamicVehicleToSpawnQueue(dynamicVehicleId);

    UpdateDynamicVehicleData(dynamicVehicleId, dynVehData);
    
    #if defined DEBUG_DYNAMIC_VEHICLES
        UpdateDynamicVehicleDebugText(dynamicVehicleId);
    #endif

    #if defined ALS_VS_OnPlayerEnterDynamicArea
        return ALS_VS_OnPlayerEnterDynamicArea(playerid, STREAMER_TAG_AREA:areaid);
    #else
        return 1;
    #endif
}
#if defined _ALS_OnPlayerEnterDynamicArea
    #undef OnPlayerEnterDynamicArea
#else
    #define _ALS_OnPlayerEnterDynamicArea
#endif
#define OnPlayerEnterDynamicArea ALS_VS_OnPlayerEnterDynamicArea
#if defined ALS_VS_OnPlayerEnterDynamicArea
    forward ALS_VS_OnPlayerEnterDynamicArea(playerid, STREAMER_TAG_AREA:areaid);
#endif

public OnPlayerLeaveDynamicArea(playerid, STREAMER_TAG_AREA:areaid) {
    new DynamicVehicle:dynamicVehicleId = DynamicVehicle:GetDynamicVehicleForAreaId(areaid);
    if (!IsValidDynamicVehicle(dynamicVehicleId))
    {
        #if defined ALS_VS_OnPlayerLeaveDynamicArea
            return ALS_VS_OnPlayerLeaveDynamicArea(playerid, areaid);
        #else
            return 1;
        #endif
    }
    
    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_playerCount] > 0)
        dynVehData[dynVeh_playerCount]--;

    if (dynVehData[dynVeh_playerCount] == 0)
    {
        dynVehData[dynVeh_despawnScheduledTime] = GetTickCount();
        list_add(s_DynamicVehiclesDespawnScheduled, _:dynamicVehicleId);
    }

    UpdateDynamicVehicleData(dynamicVehicleId, dynVehData);

    #if defined DEBUG_DYNAMIC_VEHICLES
        UpdateDynamicVehicleDebugText(dynamicVehicleId);
    #endif

    #if defined ALS_VS_OnPlayerLeaveDynamicArea
        return ALS_VS_OnPlayerLeaveDynamicArea(playerid, areaid);
    #else
        return 1;
    #endif
}
#if defined _ALS_OnPlayerLeaveDynamicArea
    #undef OnPlayerLeaveDynamicArea
#else
    #define _ALS_OnPlayerLeaveDynamicArea
#endif
#define OnPlayerLeaveDynamicArea ALS_VS_OnPlayerLeaveDynamicArea
#if defined ALS_VS_OnPlayerLeaveDynamicArea
    forward ALS_VS_OnPlayerLeaveDynamicArea(playerid, STREAMER_TAG_AREA:areaid);
#endif

#if defined VEHICLE_STREAMER_USE_ATTACHED_OBJECTS
    public OnPlayerConnect(playerid) {
        for(new Iter:i=pool_iter(g_DynamicVehicles);iter_inside(i);iter_move_next(i))
        {
            new DynamicVehicle:dynamicVehicleId = DynamicVehicle:iter_get(i);

            new 
                dynVehData[e_DYNAMIC_VEHICLE_DATA], objCount;

            GetDynamicVehicleData(dynamicVehicleId, dynVehData);

            list_count(dynVehData[dynVeh_attachedObjects], objCount);

            if (dynVehData[dynVeh_IsSpawned] && objCount > 0)
            {
                for (new Iter:x=list_iter(dynVehData[dynVeh_attachedObjects]);iter_inside(x);iter_move_next(x))
                {
                    new 
                        attachedObjectId = iter_get(x);

                    HideObjectForPlayer(playerid, attachedObjectId);
                }
            }
        }

        #if defined ALS_VS_OnPlayerConnect
            return ALS_VS_OnPlayerConnect(playerid);
        #else
            return 1;
        #endif
    }
    #if defined _ALS_OnPlayerConnect
        #undef OnPlayerConnect
    #else
        #define _ALS_OnPlayerConnect
    #endif
    #define OnPlayerConnect ALS_VS_OnPlayerConnect
    #if defined ALS_VS_OnPlayerConnect
        forward ALS_VS_OnPlayerConnect(playerid);
    #endif

    stock ALS_VS_DestroyObject(objectid) {
        // Remove from all vehicles list (if attached)
        for(new Iter:i=pool_iter(g_DynamicVehicles);iter_inside(i);iter_move_next(i))
        {
            new DynamicVehicle:dynamicVehicleId = DynamicVehicle:iter_get(i);

            new 
                dynVehData[e_DYNAMIC_VEHICLE_DATA], objCount;

            GetDynamicVehicleData(dynamicVehicleId, dynVehData);

            list_count(dynVehData[dynVeh_attachedObjects], objCount);

            if (objCount > 0)
            {
                new index = list_find(dynVehData[dynVeh_attachedObjects], objectid);
                if (index != -1)
                {
                    list_remove(dynVehData[dynVeh_attachedObjects], index);
                    UpdateDynamicVehicleData(dynamicVehicleId, dynVehData);
                    break;
                }
            }
        }
    
        return DestroyObject(objectid);
    }
    #if defined _ALS_DestroyObject
        #undef DestroyObject
    #else
        #define _ALS_DestroyObject
    #endif
    #define DestroyObject ALS_VS_DestroyObject

    stock ALS_VS_DestroyDynamicObject(objectid) {
        // Remove from all vehicles list (if attached)
        for(new Iter:i=pool_iter(g_DynamicVehicles);iter_inside(i);iter_move_next(i))
        {
            new DynamicVehicle:dynamicVehicleId = DynamicVehicle:iter_get(i);

            new 
                dynVehData[e_DYNAMIC_VEHICLE_DATA], objCount;

            GetDynamicVehicleData(dynamicVehicleId, dynVehData);

            list_count(dynVehData[dynVeh_attachedDynamicObjects], objCount);

            if (objCount > 0)
            {
                new index = list_find(dynVehData[dynVeh_attachedDynamicObjects], objectid);
                if (index != -1)
                {
                    list_remove(dynVehData[dynVeh_attachedDynamicObjects], index);
                    UpdateDynamicVehicleData(dynamicVehicleId, dynVehData);
                    break;
                }
            }
        }
    
        return DestroyDynamicObject(objectid);
    }
    #if defined _ALS_DestroyDynamicObject
        #undef DestroyDynamicObject
    #else
        #define _ALS_DestroyDynamicObject
    #endif
    #define DestroyDynamicObject ALS_VS_DestroyDynamicObject
#endif

/* Vehicle functions */

#if defined VEHICLE_STREAMER_USE_ATTACHED_OBJECTS
stock AttachObjectToDynamicVehicle(objectid, DynamicVehicle:dynamicVehicleId, Float:offsetX, Float:offsetY, Float:offsetZ, Float:rotationX, Float:rotationY, Float:rotationZ)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    if (!IsValidObject(objectid))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    new
        array[e_ATTACHED_OBJECT_DATA];
    array[attachedObj_ObjectId] = objectid;
    array[attachedObj_OffsetX] = offsetX;
    array[attachedObj_OffsetY] = offsetY;
    array[attachedObj_OffsetZ] = offsetZ;
    array[attachedObj_RotationX] = rotationX;
    array[attachedObj_RotationY] = rotationY;
    array[attachedObj_RotationZ] = rotationZ;

    list_add_arr(dynVehData[dynVeh_attachedObjects], array, .size = 7);

    UpdateDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
    {
        AttachObjectToVehicle(
            objectid,
            dynVehData[dynVeh_internalVehicleId],
            offsetX, offsetY, offsetZ,
            rotationX, rotationY, rotationZ
        );
    }
    else
    { // if car is not spawned, object should not be visible
        vehicle_streamer_loop_all_players(new playerid) {
            HideObjectForPlayer(playerid, objectid);
        }
    }

    return 1;
}

stock AttachDynamicObjectToDynamicVehicle(STREAMER_TAG_OBJECT:dynamicObjectId, DynamicVehicle:vehicleDynamicId, Float:offsetX, Float:offsetY, Float:offsetZ, Float:rotationX, Float:rotationY, Float:rotationZ)
{
    if (!IsValidDynamicVehicle(dynamicObjectId))
        return 0;

    if (!IsValidDynamicObject(dynamicObjectId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];
    
    GetDynamicVehicleData(vehicleDynamicId, dynVehData);

    new array[e_ATTACHED_OBJECT_DATA];
    array[attachedObj_ObjectId] = dynamicObjectId;
    array[attachedObj_OffsetX] = offsetX;
    array[attachedObj_OffsetY] = offsetY;
    array[attachedObj_OffsetZ] = offsetZ;
    array[attachedObj_RotationX] = rotationX;
    array[attachedObj_RotationY] = rotationY;
    array[attachedObj_RotationZ] = rotationZ;

    list_add_arr(dynVehData[dynVeh_attachedObjects], array, .size = 7);

    UpdateDynamicVehicleData(vehicleDynamicId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
    {
        AttachDynamicObjectToVehicle(
            dynamicObjectId,
            dynVehData[dynVeh_internalVehicleId],
            offsetX, offsetY, offsetZ,
            rotationX, rotationY, rotationZ
        );
    }
    else
    { // if car is not spawned, object should not be visible
        Streamer_SetIntData(STREAMER_TYPE_OBJECT, dynamicObjectId, E_STREAMER_AREA_ID, s_dynObjHiddenAreaId); 
    }

}
#endif

stock bool:IsDynamicVehicleStreamedIn(DynamicVehicle:dynamicVehicleId)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    return dynVehData[dynVeh_IsSpawned];
}

stock bool:IsDynamicVehicleStreamedInForPlayer(DynamicVehicle:dynamicVehicleId, playerid)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (!dynVehData[dynVeh_IsSpawned])
        return 0;

    return IsPlayerInDynamicArea(playerid, dynVehData[dynVeh_areaId]);
}

stock bool:GetDynamicVehiclePos(DynamicVehicle:dynamicVehicleId, &Float: x, &Float: y, &Float: z)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        return GetVehiclePos(dynVehData[dynVeh_internalVehicleId], x, y, z);

    x = dynVehData[dynVeh_CurrentX];
    y = dynVehData[dynVeh_CurrentY];
    z = dynVehData[dynVeh_CurrentZ];

    return 1;
}

stock bool:SetDynamicVehiclePos(DynamicVehicle:dynamicVehicleId, Float:x, Float:y, Float:z)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return false;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        SetVehiclePos(dynVehData[dynVeh_internalVehicleId], x, y, z);

    dynVehData[dynVeh_CurrentX] = x;
    dynVehData[dynVeh_CurrentY] = y;
    dynVehData[dynVeh_CurrentZ] = z;

    Streamer_SetFloatData(STREAMER_TYPE_AREA, dynVehData[dynVeh_areaId], E_STREAMER_X, x);
    Streamer_SetFloatData(STREAMER_TYPE_AREA, dynVehData[dynVeh_areaId], E_STREAMER_Y, y);
    Streamer_SetFloatData(STREAMER_TYPE_AREA, dynVehData[dynVeh_areaId], E_STREAMER_Z, z);

    UpdateDynamicVehicleData(dynamicVehicleId, dynVehData);

    #if defined DEBUG_DYNAMIC_VEHICLES
        UpdateDynamicVehicleDebugText(dynamicVehicleId);
    #endif

    return true;
}

stock bool:GetDynamicVehicleZAngle(DynamicVehicle:dynamicVehicleId, &Float:angle)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        return GetVehicleZAngle(dynVehData[dynVeh_internalVehicleId], angle);

    angle = dynVehData[dynVeh_CurrentAngle];
    return 1;
}

stock bool:GetDynamicVehicleRotationQuat(DynamicVehicle:dynamicVehicleId, &Float:w, &Float:x, &Float:y, &Float:z)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return false;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        return GetVehicleRotationQuat(dynVehData[dynVeh_internalVehicleId], w, x, y, z);

    // If not spawned, calculate from Z angle only - I'm not restoring the full quaternion on spawn, so this is needed
    new Float:halfAngle = dynVehData[dynVeh_CurrentAngle] * 0.5;
    w = floatcos(halfAngle, degrees);
    x = 0.0;
    y = 0.0;
    z = floatsin(halfAngle, degrees);
    return true;
}

stock Float:GetDynamicVehicleDistanceFromPoint(DynamicVehicle:dynamicVehicleId, Float:x, Float:y, Float:z)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0.0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];
    
    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        return GetVehicleDistanceFromPoint(dynVehData[dynVeh_internalVehicleId], x, y, z);

    return floatsqroot(
        floatpower(dynVehData[dynVeh_CurrentX] - x, 2.0) +
        floatpower(dynVehData[dynVeh_CurrentY] - y, 2.0) +
        floatpower(dynVehData[dynVeh_CurrentZ] - z, 2.0)
    );
}

stock bool:SetDynamicVehicleZAngle(DynamicVehicle:dynamicVehicleId, Float:angle)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        SetVehicleZAngle(dynVehData[dynVeh_internalVehicleId], angle);

    dynVehData[dynVeh_CurrentAngle] = angle;

    UpdateDynamicVehicleData(dynamicVehicleId, dynVehData);

    #if defined DEBUG_DYNAMIC_VEHICLES
        UpdateDynamicVehicleDebugText(dynamicVehicleId);
    #endif

    return 1;
}

stock bool:SetDynamicVehicleParamsForPlayer(DynamicVehicle:dynamicVehicleId, playerid, objective = -1, doors = -1)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        return SetVehicleParamsForPlayer(dynVehData[dynVeh_internalVehicleId], playerid, objective, doors);

    map_set(dynVehData[dynVeh_playerVehicleParams], playerid, {objective, doors});

    return 1;
}

stock bool:SetDynamicVehicleParamsEx(DynamicVehicle:dynamicVehicleId, engine = -1, lights = -1, alarm = -1, doors = -1, bonnet = -1, boot = -1, objective = -1)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        SetVehicleParamsEx(dynVehData[dynVeh_internalVehicleId], engine, lights, alarm, doors, bonnet, boot, objective);

    if (engine != -1)
        dynVehData[dynVeh_engine] = engine;
    if (lights != -1)
        dynVehData[dynVeh_lights] = lights;
    if (alarm != -1)
        dynVehData[dynVeh_alarm] = alarm;
    if (doors != -1)
        dynVehData[dynVeh_doors] = doors;
    if (bonnet != -1)
        dynVehData[dynVeh_bonnet] = bonnet;
    if (boot != -1)
        dynVehData[dynVeh_boot] = boot;
    if (objective != -1)
        dynVehData[dynVeh_objective] = objective;

    UpdateDynamicVehicleData(dynamicVehicleId, dynVehData);

    #if defined DEBUG_DYNAMIC_VEHICLES
        UpdateDynamicVehicleDebugText(dynamicVehicleId);
    #endif

    return 1;
}

stock bool:GetDynamicVehicleParamsEx(DynamicVehicle:dynamicVehicleId, &engine = -1, &lights = -1, &alarm = -1, &doors = -1, &bonnet = -1, &boot = -1, &objective = -1)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        return GetVehicleParamsEx(dynVehData[dynVeh_internalVehicleId], engine, lights, alarm, doors, bonnet, boot, objective);

    engine = dynVehData[dynVeh_engine];
    lights = dynVehData[dynVeh_lights];
    alarm = dynVehData[dynVeh_alarm];
    doors = dynVehData[dynVeh_doors];
    bonnet = dynVehData[dynVeh_bonnet];
    boot = dynVehData[dynVeh_boot];
    objective = dynVehData[dynVeh_objective];

    return 1;
}

stock bool:SetDynamicVehicleParamsSirenState(DynamicVehicle:dynamicVehicleId, sirenState)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        SetVehicleParamsSirenState(dynVehData[dynVeh_internalVehicleId], sirenState);

    dynVehData[dynVeh_SirenState] = sirenState;

    UpdateDynamicVehicleData(dynamicVehicleId, dynVehData);

    #if defined DEBUG_DYNAMIC_VEHICLES
        UpdateDynamicVehicleDebugText(dynamicVehicleId);
    #endif

    return 1;
}

stock GetDynamicVehicleParamsSirenState(DynamicVehicle:dynamicVehicleId)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return -1;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        return GetVehicleParamsSirenState(dynVehData[dynVeh_internalVehicleId]);

    return dynVehData[dynVeh_SirenState];
}

stock bool:SetDynamicVehicleParamsCarDoors(DynamicVehicle:dynamicVehicleId, frontLeft = -1, frontRight = -1, rearLeft = -1, rearRight = -1)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        SetVehicleParamsCarDoors(dynVehData[dynVeh_internalVehicleId], frontLeft, frontRight, rearLeft, rearRight);

    if (frontLeft != -1)
        dynVehData[dynVeh_doorFrontLeft] = frontLeft;
    if (frontRight != -1)
        dynVehData[dynVeh_doorFrontRight] = frontRight;
    if (rearLeft != -1)
        dynVehData[dynVeh_doorRearLeft] = rearLeft;
    if (rearRight != -1)
        dynVehData[dynVeh_doorRearRight] = rearRight;

    UpdateDynamicVehicleData(dynamicVehicleId, dynVehData);

    #if defined DEBUG_DYNAMIC_VEHICLES
        UpdateDynamicVehicleDebugText(dynamicVehicleId);
    #endif

    return 1;
}

stock bool:GetDynamicVehicleParamsCarDoors(DynamicVehicle:dynamicVehicleId, &frontLeft = -1, &frontRight = -1, &rearLeft = -1, &rearRight = -1)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        return GetVehicleParamsCarDoors(dynVehData[dynVeh_internalVehicleId], frontLeft, frontRight, rearLeft, rearRight);

    frontLeft = dynVehData[dynVeh_doorFrontLeft];
    frontRight = dynVehData[dynVeh_doorFrontRight];
    rearLeft = dynVehData[dynVeh_doorRearLeft];
    rearRight = dynVehData[dynVeh_doorRearRight];

    return 1;
}

stock bool:SetDynamicVehicleParamsCarWindows(DynamicVehicle:dynamicVehicleId, frontLeft = -1, frontRight = -1, rearLeft = -1, rearRight = -1)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        SetVehicleParamsCarWindows(dynVehData[dynVeh_internalVehicleId], frontLeft, frontRight, rearLeft, rearRight);

    if (frontLeft != -1)
        dynVehData[dynVeh_windowFrontLeft] = frontLeft;
    if (frontRight != -1)
        dynVehData[dynVeh_windowFrontRight] = frontRight;
    if (rearLeft != -1)
        dynVehData[dynVeh_windowRearLeft] = rearLeft;
    if (rearRight != -1)
        dynVehData[dynVeh_windowRearRight] = rearRight;

    UpdateDynamicVehicleData(dynamicVehicleId, dynVehData);

    #if defined DEBUG_DYNAMIC_VEHICLES
        UpdateDynamicVehicleDebugText(dynamicVehicleId);
    #endif

    return 1;
}

stock bool:GetDynamicVehicleParamsCarWindows(DynamicVehicle:dynamicVehicleId, &frontLeft = -1, &frontRight = -1, &rearLeft = -1, &rearRight = -1)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        return GetVehicleParamsCarWindows(dynVehData[dynVeh_internalVehicleId], frontLeft, frontRight, rearLeft, rearRight);

    frontLeft = dynVehData[dynVeh_windowFrontLeft];
    frontRight = dynVehData[dynVeh_windowFrontRight];
    rearLeft = dynVehData[dynVeh_windowRearLeft];
    rearRight = dynVehData[dynVeh_windowRearRight];

    return 1;
}

stock bool:SetDynamicVehicleToRespawn(DynamicVehicle:dynamicVehicleId)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return false;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
    {
        SetVehiclePos(dynVehData[dynVeh_internalVehicleId], 
            dynVehData[dynVeh_SpawnX],
            dynVehData[dynVeh_SpawnY],
            dynVehData[dynVeh_SpawnZ]
        );
        SetVehicleZAngle(dynVehData[dynVeh_internalVehicleId], dynVehData[dynVeh_SpawnAngle]);
        RepairVehicle(dynVehData[dynVeh_internalVehicleId]);
    }

    dynVehData[dynVeh_CurrentX] = dynVehData[dynVeh_SpawnX];
    dynVehData[dynVeh_CurrentY] = dynVehData[dynVeh_SpawnY];
    dynVehData[dynVeh_CurrentZ] = dynVehData[dynVeh_SpawnZ];
    dynVehData[dynVeh_CurrentAngle] = dynVehData[dynVeh_SpawnAngle];

    dynVehData[dynVeh_respawnTick] = GetTickCount();

    UpdateDynamicVehicleData(dynamicVehicleId, dynVehData);

    RepairDynamicVehicle(dynamicVehicleId);

    if (pawn_public_exists("OnDynamicVehicleSpawn"))
        pawn_call_public("OnDynamicVehicleSpawn", "i", _:dynamicVehicleId);

    #if defined DEBUG_DYNAMIC_VEHICLES
        UpdateDynamicVehicleDebugText(dynamicVehicleId);
    #endif
    return true;
}

stock LinkDynamicVehicleToInterior(DynamicVehicle:dynamicVehicleId, interiorId)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        LinkVehicleToInterior(dynVehData[dynVeh_internalVehicleId], interiorId);

    Streamer_SetIntData(STREAMER_TYPE_AREA, dynVehData[dynVeh_areaId], E_STREAMER_INTERIOR_ID, interiorId);

    dynVehData[dynVeh_InteriorId] = interiorId;

    UpdateDynamicVehicleData(dynamicVehicleId, dynVehData);

    #if defined DEBUG_DYNAMIC_VEHICLES
        UpdateDynamicVehicleDebugText(dynamicVehicleId);
    #endif

    return 1;
}

stock bool:AddDynamicVehicleComponent(DynamicVehicle:dynamicVehicleId, componentId)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return false;

    new
        CARMODTYPE:componentSlotId = GetVehicleComponentType(componentid);

    if (componentSlotId == CARMODTYPE_INVALID)
        return false;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        AddVehicleComponent(dynVehData[dynVeh_internalVehicleId], componentId);

    map_set(dynVehData[dynVeh_vehicleComponents], _:componentSlotId, componentId);

    UpdateDynamicVehicleData(dynamicVehicleId, dynVehData);

    #if defined DEBUG_DYNAMIC_VEHICLES
        UpdateDynamicVehicleDebugText(dynamicVehicleId);
    #endif

    return true;
}

stock bool:RemoveDynamicVehicleComponent(DynamicVehicle:dynamicVehicleId, componentId)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return false;

    new
        CARMODTYPE:componentSlotId = GetVehicleComponentType(componentid);

    if (componentSlotId == CARMODTYPE_INVALID)
        return false;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA], 
        currentComponentId;

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    currentComponentId = map_get(dynVehData[dynVeh_vehicleComponents], _:componentSlotId);

    if (currentComponentId != componentId)
        return false;

    if (dynVehData[dynVeh_IsSpawned])
        RemoveVehicleComponent(dynVehData[dynVeh_internalVehicleId], componentId);

    map_remove(dynVehData[dynVeh_vehicleComponents], _:componentSlotId);

    UpdateDynamicVehicleData(dynamicVehicleId, dynVehData);

    #if defined DEBUG_DYNAMIC_VEHICLES
        UpdateDynamicVehicleDebugText(dynamicVehicleId);
    #endif

    return true;
}

#if !defined MIXED_SPELLINGS
    #pragma deprecated Use `ChangeDynamicVehicleColours`. To silence this warning and use different spellings define `MIXED_SPELLINGS` or define `SAMP_COMPAT` for general SA-MP API compatibility.
#endif
stock ChangeDynamicVehicleColor(DynamicVehicle:dynamicVehicleId, colour1, colour2)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        ChangeVehicleColor(dynVehData[dynVeh_internalVehicleId], colour1, colour2);

    dynVehData[dynVeh_Colour1] = colour1;
    dynVehData[dynVeh_Colour2] = colour2;

    UpdateDynamicVehicleData(dynamicVehicleId, dynVehData);

    #if defined DEBUG_DYNAMIC_VEHICLES
        UpdateDynamicVehicleDebugText(dynamicVehicleId);
    #endif

    return 1;
}

stock ChangeDynamicVehicleColours(DynamicVehicle:dynamicVehicleId, colour1, colour2)
{
    return ChangeDynamicVehicleColor(dynamicVehicleId, colour1, colour2);
}

stock ChangeDynamicVehiclePaintjob(DynamicVehicle:dynamicVehicleId, paintjobid)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        ChangeVehiclePaintjob(dynVehData[dynVeh_internalVehicleId], paintjobid);

    dynVehData[dynVeh_Paintjob] = paintjobid;

    UpdateDynamicVehicleData(dynamicVehicleId, dynVehData);

    #if defined DEBUG_DYNAMIC_VEHICLES
        UpdateDynamicVehicleDebugText(dynamicVehicleId);
    #endif

    return 1;
}

stock SetDynamicVehicleHealth(DynamicVehicle:dynamicVehicleId, Float:health)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        SetVehicleHealth(dynVehData[dynVeh_internalVehicleId], health);

    dynVehData[dynVeh_CurrentHealth] = health;

    UpdateDynamicVehicleData(dynamicVehicleId, dynVehData);

    #if defined DEBUG_DYNAMIC_VEHICLES
        UpdateDynamicVehicleDebugText(dynamicVehicleId);
    #endif

    return 1;
}

stock GetDynamicVehicleHealth(DynamicVehicle:dynamicVehicleId, &Float:health)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        return GetVehicleHealth(dynVehData[dynVeh_internalVehicleId], health);

    health = dynVehData[dynVeh_CurrentHealth];
    return 1;
}

/* Trailers */
/* When Dynamic Vehicle despawn, trailer disconnects, I'm not in the mood to make it keep connected */
stock AttachTrailerToDynamicVehicle(trailerid, DynamicVehicle:dynamicVehicleId)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        return AttachTrailerToVehicle(trailerid, dynVehData[dynVeh_internalVehicleId]);

    return 0;
}

stock AttachDynamicTrailerToVehicle(DynamicVehicle:dynamicTrailerId, vehicleid)
{
    if (!IsValidDynamicVehicle(dynamicTrailerId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicTrailerId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        return AttachTrailerToVehicle(dynVehData[dynVeh_internalVehicleId], vehicleid);

    return 0;
}

stock AttachDynamicTrailerToDynamicVehicle(DynamicVehicle:dynamicTrailerId, DynamicVehicle:dynamicVehicleId)
{
    if (!IsValidDynamicVehicle(dynamicTrailerId))
        return 0;
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehDataTrailer[e_DYNAMIC_VEHICLE_DATA],
        dynVehDataVehicle[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicTrailerId, dynVehDataTrailer);
    GetDynamicVehicleData(dynamicVehicleId, dynVehDataVehicle);

    if (dynVehDataTrailer[dynVeh_IsSpawned] && dynVehDataVehicle[dynVeh_IsSpawned])
        return AttachTrailerToVehicle(dynVehDataTrailer[dynVeh_internalVehicleId], dynVehDataVehicle[dynVeh_internalVehicleId]);

    return 0;
}

stock bool:DettachTrailerFromDynamicVehicle(DynamicVehicle:dynamicVehicleId)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return false;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        return DetachTrailerFromVehicle(dynVehData[dynVeh_internalVehicleId]);

    return false;
}

stock bool:IsTrailerAttachedToDynamicVehicle(DynamicVehicle:dynamicVehicleId)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return false;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        return IsTrailerAttachedToVehicle(dynVehData[dynVeh_internalVehicleId]);

    return false;
}

stock GetDynamicVehicleTrailer(DynamicVehicle:dynamicVehicleId)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return INVALID_VEHICLE_ID;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        return GetVehicleTrailer(dynVehData[dynVeh_internalVehicleId]);

    return INVALID_VEHICLE_ID;
}

stock SetDynamicVehicleNumberPlate(DynamicVehicle:dynamicVehicleId, const numberPlate[])
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        SetVehicleNumberPlate(dynVehData[dynVeh_internalVehicleId], numberPlate);

    strcopy(dynVehData[dynVeh_NumberPlate], sizeof(dynVehData[dynVeh_NumberPlate]), numberPlate);

    UpdateDynamicVehicleData(dynamicVehicleId, dynVehData);

    #if defined DEBUG_DYNAMIC_VEHICLES
        UpdateDynamicVehicleDebugText(dynamicVehicleId);
    #endif

    return 1;
}


stock GetDynamicVehicleModel(DynamicVehicle:dynamicVehicleId)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return -1;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    return dynVehData[dynVeh_ModelId];
}

stock GetDynamicVehicleComponentInSlot(DynamicVehicle:dynamicVehicleId, CARMODTYPE:slot)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (map_has_key(dynVehData[dynVeh_vehicleComponents], _:slot))
        return map_get(dynVehData[dynVeh_vehicleComponents], _:slot);

    return 0;
}

stock RepairDynamicVehicle(DynamicVehicle:dynamicVehicleId)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        return RepairVehicle(dynVehData[dynVeh_internalVehicleId]);

    dynVehData[dynVeh_CurrentHealth] = 1000.0;
    dynVehData[dynVeh_panelState] = __TAG(VEHICLE_PANEL_STATUS):0;
    dynVehData[dynVeh_doorsState] = __TAG(VEHICLE_DOOR_STATUS):0;
    dynVehData[dynVeh_lightsState] = __TAG(VEHICLE_LIGHT_STATUS):0;
    dynVehData[dynVeh_tyresState] = __TAG(VEHICLE_TYRE_STATUS):0;

    UpdateDynamicVehicleData(dynamicVehicleId, dynVehData);

    #if defined DEBUG_DYNAMIC_VEHICLES
        UpdateDynamicVehicleDebugText(dynamicVehicleId);
    #endif

    return 1;
}

stock bool:GetDynamicVehicleVelocity(DynamicVehicle:dynamicVehicleId, &Float:x, &Float:y, &Float:z)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        return GetVehicleVelocity(dynVehData[dynVeh_internalVehicleId], x, y, z);

    x = dynVehData[dynVeh_CurrentVelocityX];
    y = dynVehData[dynVeh_CurrentVelocityY];
    z = dynVehData[dynVeh_CurrentVelocityZ];
    
    return 1;
}

stock bool:SetDynamicVehicleVelocity(DynamicVehicle:dynamicVehicleId, Float:x, Float:y, Float:z)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        return SetVehicleVelocity(dynVehData[dynVeh_internalVehicleId], x, y, z);

    dynVehData[dynVeh_CurrentVelocityX] = x;
    dynVehData[dynVeh_CurrentVelocityY] = y;
    dynVehData[dynVeh_CurrentVelocityZ] = z;

    UpdateDynamicVehicleData(dynamicVehicleId, dynVehData);

    #if defined DEBUG_DYNAMIC_VEHICLES
        UpdateDynamicVehicleDebugText(dynamicVehicleId);
    #endif

    return 1;
}

/* Note: SetDynamicVehicleAngularVelocity can only be used on spawned vehicles, since there's no getter for it */
stock bool:SetDynamicVehicleAngularVelocity(DynamicVehicle:dynamicVehicleId, Float:x, Float:y, Float:z)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        return SetVehicleAngularVelocity(dynVehData[dynVeh_internalVehicleId], x, y, z);

    return 0;
}

stock bool:GetDynamicVehicleDamageStatus(DynamicVehicle:dynamicVehicleId, &__TAG(VEHICLE_PANEL_STATUS):panelState, &__TAG(VEHICLE_DOOR_STATUS):doorsState, &__TAG(VEHICLE_LIGHT_STATUS):lightsState, &__TAG(VEHICLE_TYRE_STATUS):tyresState)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        return GetVehicleDamageStatus(dynVehData[dynVeh_internalVehicleId], panelState, doorsState, lightsState, tyresState);

    panelState = dynVehData[dynVeh_panelState];
    doorsState = dynVehData[dynVeh_doorsState];
    lightsState = dynVehData[dynVeh_lightsState];
    tyresState = dynVehData[dynVeh_tyresState];

    return 1;
}

stock bool:UpdateDynamicVehicleDamageStatus(DynamicVehicle:dynamicVehicleId, __TAG(VEHICLE_PANEL_STATUS):panelState, __TAG(VEHICLE_DOOR_STATUS):doorsState, __TAG(VEHICLE_LIGHT_STATUS):lightsState, __TAG(VEHICLE_TYRE_STATUS):tyresState)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return false;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        UpdateVehicleDamageStatus(dynVehData[dynVeh_internalVehicleId], panelState, doorsState, lightsState, tyresState);

    dynVehData[dynVeh_panelState] = panelState;
    dynVehData[dynVeh_doorsState] = doorsState;
    dynVehData[dynVeh_lightsState] = lightsState;
    dynVehData[dynVeh_tyresState] = tyresState;

    UpdateDynamicVehicleData(dynamicVehicleId, dynVehData);

    #if defined DEBUG_DYNAMIC_VEHICLES
        UpdateDynamicVehicleDebugText(dynamicVehicleId);
    #endif

    return true;
}

stock bool:SetDynamicVehicleVirtualWorld(DynamicVehicle:dynamicVehicleId, virtualWorld)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return false;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        SetVehicleVirtualWorld(dynVehData[dynVeh_internalVehicleId], virtualWorld);

    dynVehData[dynVeh_VirtualWorld] = virtualWorld;

    Streamer_SetIntData(STREAMER_TYPE_AREA, dynVehData[dynVeh_areaId], E_STREAMER_WORLD_ID, virtualWorld);

    UpdateDynamicVehicleData(dynamicVehicleId, dynVehData);

    #if defined DEBUG_DYNAMIC_VEHICLES
        UpdateDynamicVehicleDebugText(dynamicVehicleId);
    #endif

    return true;
}

stock GetDynamicVehicleVirtualWorld(DynamicVehicle:dynamicVehicleId)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return -1;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    return dynVehData[dynVeh_VirtualWorld];
}

#if defined GetVehicleSirenState
stock bool:GetDynamicVehicleSirenState(DynamicVehicle:dynamicVehicleId)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return -1;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        return GetVehicleSirenState(dynVehData[dynVeh_internalVehicleId]);

    return dynVehData[dynVeh_SirenState];
}
#endif

#if defined GetVehicleLandingGearState
stock __TAG(LANDING_GEAR_STATE):GetDynamicVehicleLandingGearState(DynamicVehicle:dynamicVehicleId)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return -1;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        return GetVehicleLandingGearState(dynVehData[dynVeh_internalVehicleId]);

    return LANDING_GEAR_STATE_DOWN; // Default state when not spawned
}
#endif

stock bool:GetDynamicVehicleSpawnInfo(DynamicVehicle:dynamicVehicleId, &Float:spawnX, &Float:spawnY, &Float:spawnZ, &Float:angle, &colour1 = 0, &colour2 = 0)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    spawnX = dynVehData[dynVeh_SpawnX];
    spawnY = dynVehData[dynVeh_SpawnY];
    spawnZ = dynVehData[dynVeh_SpawnZ];
    angle = dynVehData[dynVeh_SpawnAngle];
    colour1 = dynVehData[dynVeh_Colour1];
    colour2 = dynVehData[dynVeh_Colour2];

    return 1;
}

stock bool:SetDynamicVehicleSpawnInfo(DynamicVehicle:dynamicVehicleId, modelid, Float:spawnX, Float:spawnY, Float:spawnZ, Float:angle, colour1, colour2, respawnDelay = -2, interior = -2)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    dynVehData[dynVeh_ModelId] = modelid;
    dynVehData[dynVeh_SpawnX] = spawnX;
    dynVehData[dynVeh_SpawnY] = spawnY;
    dynVehData[dynVeh_SpawnZ] = spawnZ;
    dynVehData[dynVeh_SpawnAngle] = angle;
    dynVehData[dynVeh_Colour1] = colour1;
    dynVehData[dynVeh_Colour2] = colour2;

    if (respawnDelay != -2)
        dynVehData[dynVeh_RespawnDelay] = respawnDelay;
    if (interior != -2)
        dynVehData[dynVeh_InteriorId] = interior;

    UpdateDynamicVehicleData(dynamicVehicleId, dynVehData);

    #if defined DEBUG_DYNAMIC_VEHICLES
        UpdateDynamicVehicleDebugText(dynamicVehicleId);
    #endif

    return 1;
}

#if !defined MIXED_SPELLINGS
	#pragma deprecated Use `GetDynamicVehicleColours`. To silence this warning and use different spellings define `MIXED_SPELLINGS` or define `SAMP_COMPAT` for general SA-MP API compatibility.
#endif
stock bool:GetDynamicVehicleColor(DynamicVehicle:dynamicVehicleId, &color1, &color2)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    color1 = dynVehData[dynVeh_Colour1];
    color2 = dynVehData[dynVeh_Colour2];

    return 1;
}

stock bool:GetDynamicVehicleColours(DynamicVehicle:dynamicVehicleId, &colour1, &colour2)
{
    return GetDynamicVehicleColor(dynamicVehicleId, colour1, colour2);
}

stock GetDynamicVehiclePaintJob(DynamicVehicle:dynamicVehicleId)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return -1;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    return dynVehData[dynVeh_PaintJob];
}

stock GetDynamicVehicleInterior(DynamicVehicle:dynamicVehicleId)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return -1;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    return dynVehData[dynVeh_InteriorId];
}

#if defined GetVehicleNumberPlate
stock GetDynamicVehicleNumberPlate(DynamicVehicle:dynamicVehicleId, plate[], len = sizeof(plate))
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    strcopy(plate, len, dynVehData[dynVeh_NumberPlate]);

    return 1;
}
#endif

stock bool:SetDynamicVehicleRespawnDelay(DynamicVehicle:dynamicVehicleId, respawnDelay)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    dynVehData[dynVeh_RespawnDelay] = respawnDelay;

    UpdateDynamicVehicleData(dynamicVehicleId, dynVehData);

    #if defined DEBUG_DYNAMIC_VEHICLES
        UpdateDynamicVehicleDebugText(dynamicVehicleId);
    #endif

    return 1;
}

stock GetDynamicVehicleRespawnDelay(DynamicVehicle:dynamicVehicleId)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return -1;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    return dynVehData[dynVeh_RespawnDelay];
}

#if defined GetVehicleOccupiedTick
stock GetDynamicVehicleOccupiedTick(DynamicVehicle:dynamicVehicleId)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return -1;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    return (GetTickCount() - dynVehData[dynVeh_lastOccupiedTick]);
}
#endif

#if defined SetVehicleOccupiedTick
stock SetDynamicVehicleOccupiedTick(DynamicVehicle:dynamicVehicleId, ticks)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    dynVehData[dynVeh_lastOccupiedTick] = ticks;

    UpdateDynamicVehicleData(dynamicVehicleId, dynVehData);

    #if defined DEBUG_DYNAMIC_VEHICLES
        UpdateDynamicVehicleDebugText(dynamicVehicleId);
    #endif

    return 1;
}
#endif

#if defined GetVehicleRespawnTick
stock GetDynamicVehicleRespawnTick(DynamicVehicle:dynamicVehicleId)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return -1;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    return (GetTickCount() - dynVehData[dynVeh_respawnTick]);
}
#endif

#if defined SetVehicleRespawnTick
stock SetDynamicVehicleRespawnTick(DynamicVehicle:dynamicVehicleId, ticks)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    dynVehData[dynVeh_respawnTick] = ticks;

    UpdateDynamicVehicleData(dynamicVehicleId, dynVehData);

    #if defined DEBUG_DYNAMIC_VEHICLES
        UpdateDynamicVehicleDebugText(dynamicVehicleId);
    #endif

    return 1;
}
#endif

#if defined GetVehicleLastDriver
stock GetDynamicVehicleLastDriver(DynamicVehicle:dynamicVehicleId)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return INVALID_PLAYER_ID;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    return dynVehData[dynVeh_lastDriverPlayerId];
}
#endif

stock GetDynamicVehicleDriver(DynamicVehicle:dynamicVehicleId)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return INVALID_PLAYER_ID;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        return GetVehicleDriver(dynVehData[dynVeh_internalVehicleId]);

    return INVALID_PLAYER_ID;
}

// Equivalent to GetDynamicVehicleTower
stock GetDynamicVehicleCab(DynamicVehicle:dynamicVehicleId)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        return GetVehicleCab(dynVehData[dynVeh_internalVehicleId]);

    return 0;
}

#if defined HasVehicleBeenOccupied
stock bool:HasDynamicVehicleBeenOccupied(DynamicVehicle:dynamicVehicleId)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    return dynVehData[dynVeh_lastOccupiedTick] != 0;
}
#endif

stock bool:IsDynamicVehicleOccupied(DynamicVehicle:dynamicVehicleId)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        return IsVehicleOccupied(dynVehData[dynVeh_internalVehicleId]);

    return 0;
}

stock bool:IsDynamicVehicleDead(DynamicVehicle:dynamicVehicleId)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        return IsVehicleDead(dynVehData[dynVeh_internalVehicleId]);

    return 0;
}

stock bool:ToggleDynamicVehicleSirenEnabled(DynamicVehicle:dynamicVehicleId, bool:enabled)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        return ToggleVehicleSirenEnabled(dynVehData[dynVeh_internalVehicleId], enabled);

    dynVehData[dynVeh_SirenState] = enabled ? 1 : 0;

    return 1;
}

stock bool:GetDynamicVehicleMatrix(DynamicVehicle:dynamicVehicleId, &Float:rightX, &Float:rightY, &Float:rightZ, &Float:upX, &Float:upY, &Float:upZ, &Float:atX, &Float:atY, &Float:atZ)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        return GetVehicleMatrix(dynVehData[dynVeh_internalVehicleId], rightX, rightY, rightZ, upX, upY, upZ, atX, atY, atZ);

    // Calculate matrix from position and angle
    new Float:cosA = floatcos(dynVehData[dynVeh_CurrentAngle], degrees);
    new Float:sinA = floatsin(dynVehData[dynVeh_CurrentAngle], degrees);
    
    // Right vector
    rightX = cosA;
    rightY = sinA;
    rightZ = 0.0;

    // Up vector
    upX = 0.0;
    upY = 0.0;
    upZ = 1.0;

    // At vector
    atX = -sinA;
    atY = cosA;
    atZ = 0.0;

    return 1;
}

stock GetDynamicVehicleOccupant(DynamicVehicle:dynamicVehicleId, seatid)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return INVALID_PLAYER_ID;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        return GetVehicleOccupant(dynVehData[dynVeh_internalVehicleId], seatid);

    return INVALID_PLAYER_ID;
}

stock CountDynamicVehicleOccupants(DynamicVehicle:dynamicVehicleId)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        return CountVehicleOccupants(dynVehData[dynVeh_internalVehicleId]);

    return 0;
}

#if !defined LEGACY_SCRIPTING_API
	#pragma deprecated Use `GetDynamicVehicleCab`. To silence this warning define `LEGACY_SCRIPTING_API` or define `SAMP_COMPAT` for general SA-MP API compatibility.
#endif
stock GetDynamicVehicleTower(DynamicVehicle:dynamicVehicleId)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        return GetVehicleTower(dynVehData[dynVeh_internalVehicleId]);

    return 0;
}

stock PutPlayerInDynamicVehicle(DynamicVehicle:dynamicVehicleId, playerid, seatid)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return 0;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        return PutPlayerInVehicle(playerid, dynVehData[dynVeh_internalVehicleId], seatid);

    // TODO: Force vehicle to spawn before putting player in it

    return 0;
}

stock bool:IsPlayerInDynamicVehicle(DynamicVehicle:dynamicVehicleId, playerid)
{
    if (!IsValidDynamicVehicle(dynamicVehicleId))
        return false;

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];

    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (dynVehData[dynVeh_IsSpawned])
        return IsPlayerInVehicle(playerid, dynVehData[dynVeh_internalVehicleId]);

    return false;
}

/* Callbacks */
forward OnDynamicVehicleSirenStateChange(playerid, DynamicVehicle:dynamicVehicleId, newstate);
public OnVehicleSirenStateChange(playerid, vehicleid, newstate) {
    new DynamicVehicle:dynamicVehicleId = GetDynamicVehicleFromInternalVehicleId(vehicleid);
    if (dynamicVehicleId == INVALID_DYNAMIC_VEHICLE_ID)
    {
        #if defined ALS_VS_OnVehicleSirenStateChange
            return ALS_VS_OnVehicleSirenStateChange(playerid, vehicleid, newstate);
        #else
            return 1;
        #endif
    }

    if (pawn_public_exists("OnDynamicVehicleirenStateChange"))
        pawn_call_public("OnDynamicVehicleSirenStateChange", "iii", playerid, _:dynamicVehicleId, newstate);

    return 1;
}
#if defined _ALS_OnVehicleSirenStateChange
    #undef OnVehicleSirenStateChange
#else
    #define _ALS_OnVehicleSirenStateChange
#endif
#define OnVehicleSirenStateChange ALS_VS_OnVehicleSirenStateChange
#if defined ALS_VS_OnVehicleSirenStateChange
    forward ALS_VS_OnVehicleSirenStateChange(playerid, vehicleid, newstate);
#endif

/* isInternalStream is true only when the vehicle is streamed by the include itself (CreateVehicle inside the include). /
/ when isInternalStream is true, forplayerid will be INVALID_PLAYER_ID */
forward OnDynamicVehicleStreamIn(DynamicVehicle:dynamicVehicleId, forplayerid, bool:isInternalStream);
public OnVehicleStreamIn(vehicleid, forplayerid) {
    new DynamicVehicle:dynamicVehicleId = GetDynamicVehicleFromInternalVehicleId(vehicleid);
    if (dynamicVehicleId == INVALID_DYNAMIC_VEHICLE_ID)
    {
        #if defined ALS_VS_OnVehicleStreamIn
            return ALS_VS_OnVehicleStreamIn(vehicleid, forplayerid);
        #else
            return 1;
        #endif
    }

    if (pawn_public_exists("OnDynamicVehicleStreamIn"))
        pawn_call_public("OnDynamicVehicleStreamIn", "iii", _:dynamicVehicleId, forplayerid, 0);

    return 1;
}
#if defined _ALS_OnVehicleStreamIn
    #undef OnVehicleStreamIn
#else
    #define _ALS_OnVehicleStreamIn
#endif
#define OnVehicleStreamIn ALS_VS_OnVehicleStreamIn
#if defined ALS_VS_OnVehicleStreamIn
    forward ALS_VS_OnVehicleStreamIn(vehicleid, forplayerid);
#endif

/* isInternalStream is true only when the vehicle is streamed by the include itself (DestroyVehicle inside the include). /
/ when isInternalStream is true, forplayerid will be INVALID_PLAYER_ID */
forward OnDynamicVehicleStreamOut(DynamicVehicle:dynamicVehicleId, forplayerid, bool:isInternalStream);
public OnVehicleStreamOut(vehicleid, forplayerid) {
    new DynamicVehicle:dynamicVehicleId = GetDynamicVehicleFromInternalVehicleId(vehicleid);
    if (dynamicVehicleId == INVALID_DYNAMIC_VEHICLE_ID)
    {
        #if defined ALS_VS_OnVehicleStreamOut
            return ALS_VS_OnVehicleStreamOut(vehicleid, forplayerid);
        #else
            return 1;
        #endif
    }

    if (pawn_public_exists("OnDynamicVehicleStreamOut"))
        pawn_call_public("OnDynamicVehicleStreamOut", "iii", _:dynamicVehicleId, forplayerid, 0);

    return 1;
}
#if defined _ALS_OnVehicleStreamOut
    #undef OnVehicleStreamOut
#else
    #define _ALS_OnVehicleStreamOut
#endif
#define OnVehicleStreamOut ALS_VS_OnVehicleStreamOut
#if defined ALS_VS_OnVehicleStreamOut
    forward ALS_VS_OnVehicleStreamOut(vehicleid, forplayerid);
#endif

forward OnPlayerEnterDynamicVehicle(DynamicVehicle:dynamicVehicleId, playerid, ispassenger);
public OnPlayerEnterVehicle(playerid, vehicleid, ispassenger) {
    new DynamicVehicle:dynamicVehicleId = GetDynamicVehicleFromInternalVehicleId(vehicleid);
    if (dynamicVehicleId == INVALID_DYNAMIC_VEHICLE_ID)
    {
        #if defined ALS_VS_OnPlayerEnterVehicle
            return ALS_VS_OnPlayerEnterVehicle(playerid, vehicleid, ispassenger);
        #else
            return 0;
        #endif
    }

    new
        dynVehData[e_DYNAMIC_VEHICLE_DATA];
    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    dynVehData[dynVeh_lastOccupiedTick] = GetTickCount();

    UpdateDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (pawn_public_exists("OnDynamicVehicleStreamIn"))
        pawn_call_public("OnPlayerEnterDynamicVehicle", "iii", _:dynamicVehicleId, playerid, ispassenger);

    return 1;
}
#if defined _ALS_OnPlayerEnterVehicle
    #undef OnPlayerEnterVehicle
#else
    #define _ALS_OnPlayerEnterVehicle
#endif
#define OnPlayerEnterVehicle ALS_VS_OnPlayerEnterVehicle
#if defined ALS_VS_OnPlayerEnterVehicle
    forward ALS_VS_OnPlayerEnterVehicle(playerid, vehicleid, ispassenger);
#endif

forward OnPlayerExitDynamicVehicle(DynamicVehicle:dynamicVehicleId, playerid);
public OnPlayerExitVehicle(playerid, vehicleid) {
    new DynamicVehicle:dynamicVehicleId = GetDynamicVehicleFromInternalVehicleId(vehicleid);
    if (dynamicVehicleId == INVALID_DYNAMIC_VEHICLE_ID)
    {
        #if defined ALS_VS_OnPlayerExitVehicle
            return ALS_VS_OnPlayerExitVehicle(playerid, vehicleid);
        #else
            return 0;
        #endif
    }

    new
        dynVehData[e_DYNAMIC_VEHICLE_DATA];
    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    dynVehData[dynVeh_lastOccupiedTick] = GetTickCount();

    UpdateDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (pawn_public_exists("OnDynamicVehicleStreamOut"))
        pawn_call_public("OnPlayerExitDynamicVehicle", "ii", _:dynamicVehicleId, playerid);

    return 1;
}
#if defined _ALS_OnPlayerExitVehicle
    #undef OnPlayerExitVehicle
#else
    #define _ALS_OnPlayerExitVehicle
#endif
#define OnPlayerExitVehicle ALS_VS_OnPlayerExitVehicle
#if defined ALS_VS_OnPlayerExitVehicle
    forward ALS_VS_OnPlayerExitVehicle(playerid, vehicleid);
#endif

forward OnDynamicVehicleSpawn(DynamicVehicle:dynamicVehicleId);
public OnVehicleSpawn(vehicleid) {
    new DynamicVehicle:dynamicVehicleId = GetDynamicVehicleFromInternalVehicleId(vehicleid);
    if (dynamicVehicleId == INVALID_DYNAMIC_VEHICLE_ID)
    {
        #if defined ALS_VS_OnVehicleSpawn
            return ALS_VS_OnVehicleSpawn(vehicleid);
        #else
            return 1;
        #endif
    }

    // Vehicle spawns for dynamic vehicles are handled internally
    SetDynamicVehicleToRespawn(dynamicVehicleId); // this is just used to reset the vars

    return 1;
}
#if defined _ALS_OnVehicleSpawn
    #undef OnVehicleSpawn
#else
    #define _ALS_OnVehicleSpawn
#endif
#define OnVehicleSpawn ALS_VS_OnVehicleSpawn
#if defined ALS_VS_OnVehicleSpawn
    forward ALS_VS_OnVehicleSpawn(vehicleid);
#endif

forward OnDynamicVehicleDeath(DynamicVehicle:dynamicVehicleId, killerid);
public OnVehicleDeath(vehicleid, killerid) {
    new DynamicVehicle:dynamicVehicleId = GetDynamicVehicleFromInternalVehicleId(vehicleid);
    if (dynamicVehicleId == INVALID_DYNAMIC_VEHICLE_ID)
    {
        #if defined ALS_VS_OnVehicleDeath
            return ALS_VS_OnVehicleDeath(vehicleid, killerid);
        #else
            return 1;
        #endif
    }

    if (pawn_public_exists("OnDynamicVehicleDeath"))
        pawn_call_public("OnDynamicVehicleDeath", "ii", _:dynamicVehicleId, killerid);

    return 1;
}
#if defined _ALS_OnVehicleDeath
    #undef OnVehicleDeath
#else
    #define _ALS_OnVehicleDeath
#endif
#define OnVehicleDeath ALS_VS_OnVehicleDeath
#if defined ALS_VS_OnVehicleDeath
    forward ALS_VS_OnVehicleDeath(vehicleid, killerid);
#endif

forward OnDynamicVehicleMod(DynamicVehicle:dynamicVehicleId, playerid, componentid);
public OnVehicleMod(playerid, vehicleid, componentid) {
    new DynamicVehicle:dynamicVehicleId = GetDynamicVehicleFromInternalVehicleId(vehicleid);
    if (dynamicVehicleId == INVALID_DYNAMIC_VEHICLE_ID)
    {
        #if defined ALS_VS_OnVehicleMod
            return ALS_VS_OnVehicleMod(playerid, vehicleid, componentid);
        #else
            return 1;
        #endif
    }

    if (pawn_public_exists("OnDynamicVehicleMod"))
        pawn_call_public("OnDynamicVehicleMod", "iii", _:dynamicVehicleId, playerid, componentid);

    return 1;
}
#if defined _ALS_OnVehicleMod
    #undef OnVehicleMod
#else
    #define _ALS_OnVehicleMod
#endif
#define OnVehicleMod ALS_VS_OnVehicleMod
#if defined ALS_VS_OnVehicleMod
    forward ALS_VS_OnVehicleMod(playerid, vehicleid, componentid);
#endif

forward OnDynamicVehiclePaintjob(DynamicVehicle:dynamicVehicleId, playerid, paintjobid);
public OnVehiclePaintjob(playerid, vehicleid, paintjobid) {
    new DynamicVehicle:dynamicVehicleId = GetDynamicVehicleFromInternalVehicleId(vehicleid);
    if (dynamicVehicleId == INVALID_DYNAMIC_VEHICLE_ID)
    {
        #if defined ALS_VS_OnVehiclePaintjob
            return ALS_VS_OnVehiclePaintjob(playerid, vehicleid, paintjobid);
        #else
            return 1;
        #endif
    }

    new 
        dynVehData[e_DYNAMIC_VEHICLE_DATA];
    GetDynamicVehicleData(dynamicVehicleId, dynVehData);

    dynVehData[dynVeh_PaintJob] = paintjobid;

    UpdateDynamicVehicleData(dynamicVehicleId, dynVehData);

    if (pawn_public_exists("OnDynamicVehiclePaintjob"))
        pawn_call_public("OnDynamicVehiclePaintjob", "iii", _:dynamicVehicleId, playerid, paintjobid);

    return 1;
}
#if defined _ALS_OnVehiclePaintjob
    #undef OnVehiclePaintjob
#else
    #define _ALS_OnVehiclePaintjob
#endif
#define OnVehiclePaintjob ALS_VS_OnVehiclePaintjob
#if defined ALS_VS_OnVehiclePaintjob
    forward ALS_VS_OnVehiclePaintjob(playerid, vehicleid, paintjobid);
#endif

forward OnDynamicVehicleRespray(DynamicVehicle:dynamicVehicleId, playerid, color1, color2);
public OnVehicleRespray(playerid, vehicleid, color1, color2) {
    new DynamicVehicle:dynamicVehicleId = GetDynamicVehicleFromInternalVehicleId(vehicleid);
    if (dynamicVehicleId == INVALID_DYNAMIC_VEHICLE_ID)
    {
        #if defined ALS_VS_OnVehicleRespray
            return ALS_VS_OnVehicleRespray(playerid, vehicleid, color1, color2);
        #else
            return 1;
        #endif
    }

    if (pawn_public_exists("OnDynamicVehicleRespray"))
        pawn_call_public("OnDynamicVehicleRespray", "iiii", _:dynamicVehicleId, playerid, color1, color2);

    return 1;
}
#if defined _ALS_OnVehicleRespray
    #undef OnVehicleRespray
#else
    #define _ALS_OnVehicleRespray
#endif
#define OnVehicleRespray ALS_VS_OnVehicleRespray
#if defined ALS_VS_OnVehicleRespray
    forward ALS_VS_OnVehicleRespray(playerid, vehicleid, color1, color2);
#endif

forward OnDynamicVehicleDamageStatusUpdate(DynamicVehicle:dynamicVehicleId, playerid);
public OnVehicleDamageStatusUpdate(vehicleid, playerid) {
    new DynamicVehicle:dynamicVehicleId = GetDynamicVehicleFromInternalVehicleId(vehicleid);
    if (dynamicVehicleId == INVALID_DYNAMIC_VEHICLE_ID)
    {
        #if defined ALS_VS_OnVehicleDamageStatusUpdate
            return ALS_VS_OnVehicleDamageStatusUpdate(vehicleid, playerid);
        #else
            return 1;
        #endif
    }

    if (pawn_public_exists("OnDynamicVehicleDamageStatusUpdate"))
        pawn_call_public("OnDynamicVehicleDamageStatusUpdate", "ii", _:dynamicVehicleId, playerid);

    return 1;
}
#if defined _ALS_OnVehicleDamageStatusUpdate
    #undef OnVehicleDamageStatusUpdate
#else
    #define _ALS_OnVehicleDamageStatusUpdate
#endif
#define OnVehicleDamageStatusUpdate ALS_VS_OnVehicleDamageStatusUpdate
#if defined ALS_VS_OnVehicleDamageStatusUpdate
    forward ALS_VS_OnVehicleDamageStatusUpdate(vehicleid, playerid);
#endif

forward OnDynamicVehicleUnoccupiedUpdate(DynamicVehicle:dynamicVehicleId, playerid, passenger_seat, Float:new_x, Float:new_y, Float:new_z, Float:vel_x, Float:vel_y, Float:vel_z);
public OnUnoccupiedVehicleUpdate(vehicleid, playerid, passenger_seat, Float:new_x, Float:new_y, Float:new_z, Float:vel_x, Float:vel_y, Float:vel_z) {
    new DynamicVehicle:dynamicVehicleId = GetDynamicVehicleFromInternalVehicleId(vehicleid);
    if (dynamicVehicleId == INVALID_DYNAMIC_VEHICLE_ID)
    {
        #if defined ALS_VS_OnUnoccupiedVehicleUpdate
            return ALS_VS_OnUnoccupiedVehicleUpdate(vehicleid, playerid, passenger_seat, new_x, new_y, new_z, vel_x, vel_y, vel_z);
        #else
            return 1;
        #endif
    }

    if (pawn_public_exists("OnDynamicVehicleUnoccupiedUpdate"))
        pawn_call_public("OnUnoccupiedVehicleUpdate", "iiiffffff", _:dynamicVehicleId, playerid, passenger_seat, new_x, new_y, new_z, vel_x, vel_y, vel_z);

    return 1;
}
#if defined _ALS_OnUnoccupiedVehicleUpdate
    #undef OnUnoccupiedVehicleUpdate
#else
    #define _ALS_OnUnoccupiedVehicleUpdate
#endif
#define OnUnoccupiedVehicleUpdate ALS_VS_OnUnoccupiedVehicleUpdate
#if defined ALS_VS_OnUnoccupiedVehicleUpdate
    forward ALS_VS_OnUnoccupiedVehicleUpdate(vehicleid, playerid, passenger_seat, Float:new_x, Float:new_y, Float:new_z, Float:vel_x, Float:vel_y, Float:vel_z);
#endif

#define BULLET_HIT_TYPE_DYNAMIC_VEHICLE          (__TAG(BULLET_HIT_TYPE):5)

public OnPlayerWeaponShot(playerid, __TAG(WEAPON):weaponid, __TAG(BULLET_HIT_TYPE):hittype, hitid, Float:fX, Float:fY, Float:fZ) {
    if (hittype != BULLET_HIT_TYPE_VEHICLE)
    {
        #if defined ALS_VS_OnPlayerWeaponShot
            return ALS_VS_OnPlayerWeaponShot(playerid, weaponid, hittype, hitid, fX, fY, fZ);
        #else
            return 1;
        #endif
    }
    new DynamicVehicle:dynamicVehicleId = GetDynamicVehicleFromInternalVehicleId(hitid);
    if (dynamicVehicleId == INVALID_DYNAMIC_VEHICLE_ID)
    {
        #if defined ALS_VS_OnPlayerWeaponShot
            return ALS_VS_OnPlayerWeaponShot(playerid, weaponid, hittype, hitid, fX, fY, fZ);
        #else
            return 1;
        #endif
    }

    OnPlayerWeaponShot(playerid, weaponid, BULLET_HIT_TYPE_DYNAMIC_VEHICLE, _:dynamicVehicleId, fX, fY, fZ);

    return 1;
}
#if defined _ALS_OnPlayerWeaponShot
    #undef OnPlayerWeaponShot
#else
    #define _ALS_OnPlayerWeaponShot
#endif
#define OnPlayerWeaponShot ALS_VS_OnPlayerWeaponShot
#if defined ALS_VS_OnPlayerWeaponShot
    forward ALS_VS_OnPlayerWeaponShot(playerid, __TAG(WEAPON):weaponid, __TAG(BULLET_HIT_TYPE):hittype, hitid, Float:fX, Float:fY, Float:fZ);
#endif

#if defined SHOULD_UNDEF_DEBUG_DYNAMIC_VEHICLES
    #undef SHOULD_UNDEF_DEBUG_DYNAMIC_VEHICLES
    #undef DEBUG_DYNAMIC_VEHICLES
#endif
