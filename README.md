# Samp Vehicle Streamer
This vehicle streamers is a work-around for the samp 2000 created vehicles limit. It creates vehicles when there are players nearby, and deletes them once all players go away.

It uses PawnPlus plugin by IS4 and Streamer plugin by Incognito.
You can download them here:
- PawnPlus: https://github.com/IS4Code/PawnPlus/releases (Tested on v1.5.2)
- Streamer: https://github.com/samp-incognito/samp-streamer-plugin/releases (Tested on v2.9.6)

# Contents
- [Contents](https://github.com/leamir/samp-vehicle-streamer#contents)
- [Defines](https://github.com/leamir/samp-vehicle-streamer#defines)
- [Functions](https://github.com/leamir/samp-vehicle-streamer#functions)
- [Callbacks](https://github.com/leamir/samp-vehicle-streamer#callbacks)

# Defines
### ⚠️ All defines must be placed before the vehicle streamer is included.

```pawn
#define DYNAMIC_VEHICLES_SPAWN_QUEUE_PROCESS_INTERVAL 250
```
The `DYNAMIC_VEHICLES_SPAWN_QUEUE_PROCESS_INTERVAL`(Default: `250`) define controls when the spawn queue is processed. A longer time will make vehicles take longer to spawn when players get closer\*
- This value is a interval in miliseconds.
\* Only 1 spawn queue loop runs, it processess all vehicles in the queue, so it's possible, although unlikely that a player will get near a car when the queue is being processed, which would cause it to immediately spawn as soon as this vehicle reaches the start of the queue. This has a higher risk of hapenning if the vehicle reaches the vehicle limit.
- The spawn queue spawns vehicles in order
- Each run it will create as many vehicles as possible.
- If the vehicle wasn't created due to reaching MAX_VEHICLES, the queue stops processing until the next interval.
- The vehicle that coudn't be created remains at the front of the queue.

---

```pawn
#define DYNAMIC_VEHICLES_DESPAWN_CHECK_INTERVAL 5000
```
The `DYNAMIC_VEHICLES_DESPAWN_CHECK_INTERVAL`(Default: `5000`) define controls the minimum time a vehicle needs to be unnatended (no players in it's stream range) for it to dispawn. 
It is the **absolute minimum** time the vehicle needs to be unnatended for it to despawn, it may take a couple more miliseconds for it to actually be un-streamed by the include. 
- Every interval, checks for vehicles that need to be despawned.
- A vehicle needs to be despawned when there are no players in its area;
- Vehicles need to have no players around for at least DYNAMIC_VEHICLES_DESPAWN_CHECK_INTERVAL milliseconds before being despawned.

---

```pawn
#define STREAMER_EXTRA_ID_DYNAMIC_VEHICLE E_STREAMER_CUSTOM(1)
```
The `STREAMER_EXTRA_ID_DYNAMIC_VEHICLE`(Default: `E_STREAMER_CUSTOM(1)`) define controls what streamer extra ID should be used internally in the areas that detect player proximity for streaming. Any one will work, as long as it's not being used in your gamemode.
- Just make sure you're not using this extra ID for something else in your gamemode.
- It's used to link a dynamic vehicle to its area.

---

The `VEHICLE_STREAMER_USE_ATTACHED_OBJECTS`(Default: `NOT DEFINED`) define controls if the include will use attached objects. This define was added because there's likely a performance impact due to the way attached objects were implemented.
- You need to DEFINE this if you want to use it, doesn't matter the value.
- I haven't really tested whether there's noticiable overhead or not, but don't want to risk it (there probably is)
- **For use of attached objects, you need to have `[Hide/Show]ObjectForPlayer`. This can be achieved using either [open.mp](https://open.mp) or [YSF plugin by IS4](https://github.com/IS4Code/YSF) if you wish to remain on SAMP.**
```cpp
#define VEHICLE_STREAMER_USE_ATTACHED_OBJECTS
```

# Functions
### Some of this functions have a sa-mp/open.mp equivalent for non-dynamic vehicles. Those are expected to mirror the behaviour from the non-dynamic vehicles.

```pawn
stock IsValidDynamicVehicle(DynamicVehicle:dynamicVehicleId)
```
Returns `true` if a dynamic vehicle id is valid, false otherwise.

You're probably looking for `GetDynamicVehicleFromInternalVehicleId` for getting dynamic vehicle id from a normal vehicle id. 

Example use:
```pawn
new vehicleid = CreateVehicle(411, 0.0, 0.0, 15.0, -1, -1);
new DynamicVehicle:dynamicVehicleId = GetDynamicVehicleFromInternalVehicleId(vehicleid);

if (!IsValidDynamicVehicle(dynamicVehicleId))
{
    SendClientMessageToAll(-1, "Vehicle is not a dynamic vehicle.");
}
```

---

```pawn
stock GetDynamicVehicleData(DynamicVehicle:dynamicVehicleId, array[])
```
Returns the internal array for that vehicle id.

Example use:
```pawn
new
    dynVehData[e_DYNAMIC_VEHICLE_DATA];
    
GetDynamicVehicleData(DynamicVehicle:0, dynVehData);

if (dynVehData[dynVeh_IsSpawned])
{
    SendClientMessageToAll(-1, "Dynamic vehicle 0 is spawned");
}
```

---

```pawn
stock UpdateDynamicVehicleData(DynamicVehicle:dynamicVehicleId, const array[])
```
Setter for `GetDynamicVehicleData`. You can modify the array returned by `GetDynamicVehicleData` and update it internally using this. Most changes only apply once the dynamic vehicle respawns.

---

```pawn
stock DynamicVehicle:GetDynamicVehicleFromInternalVehicleId(internalVehicleId)
```
Returns the internal ID for a vehicleid if it is a dynamic vehicle, `INVALID_DYNAMIC_VEHICLE_ID` otherwise.

Example use:
```pawn
new vehicleid = CreateVehicle(411, 0.0, 0.0, 15.0, -1, -1);
new DynamicVehicle:dynamicVehicleId = GetDynamicVehicleFromInternalVehicleId(vehicleid);

if (!IsValidDynamicVehicle(dynamicVehicleId))
{
    SendClientMessageToAll(-1, "Vehicle is not a dynamic vehicle.");
}
```

---

```pawn
stock DynamicVehicle:GetDynamicVehicleForAreaId(STREAMER_TAG_AREA:areaId)
```
Returns the Dynamic Vehicle that is attached to the passed areaid, `INVALID_DYNAMIC_VEHICLE_ID` if the area is not attached to any dynamic vehicle.

---

```pawn
stock DynamicVehicle:CreateDynamicVehicle(modelid, Float:x, Float:y, Float:z, Float:angle, colour1 = -1, colour2 = -1, respawnDelay = -1, bool:addSiren = false, Float:streamRadius = 300.0)
```
* streamRadius is the radius any player needs to go near for the vehicle to be streamed.
* See the wiki for more information on the other params.

---

```pawn
stock DestroyDynamicVehicle(DynamicVehicle:dynamicVehicleId)
```
* See the wiki for more information on this function.

---

```pawn
static AddDynamicVehicleToSpawnQueue(DynamicVehicle:dynamicVehicleId)
```
Adds a dynamic vehicle to the spawn queue, even if no players are nearby.
* ⚠️ If this function is used with no players nearby, vehicle will not be added to the despawn queue until a player goes close and moves away from it.

---

```pawn
stock AttachObjectToDynamicVehicle(objectid, DynamicVehicle:dynamicVehicleId, Float:offsetX, Float:offsetY, Float:offsetZ, Float:rotationX, Float:rotationY, Float:rotationZ)
```
* ⚠️ Only available if `VEHICLE_STREAMER_USE_ATTACHED_OBJECTS` is defined.
* See the wiki for more information on this function.

---

```pawn
stock AttachDynamicObjectToDynamicVehicle(STREAMER_TAG_OBJECT:dynamicObjectId, DynamicVehicle:vehicleDynamicId, Float:offsetX, Float:offsetY, Float:offsetZ, Float:rotationX, Float:rotationY, Float:rotationZ)
```
* ⚠️ Only available if `VEHICLE_STREAMER_USE_ATTACHED_OBJECTS` is defined.
* See the wiki for more information on this function.

---

```pawn
stock bool:IsDynamicVehicleStreamedIn(DynamicVehicle:dynamicVehicleId)
```
* ⚠️ This function defers from the sa-mp/open.mp definition.
Returns true if the vehicle is `SPAWNED`, and thus have at least 1 player near it.

---

```pawn
stock bool:IsDynamicVehicleStreamedInForPlayer(DynamicVehicle:dynamicVehicleId, playerid)
```
* Same as `IsDynamicVehicleStreamedIn`, but also checks if player is in the vehicle's stream area.

---

```pawn
stock bool:GetDynamicVehiclePos(DynamicVehicle:dynamicVehicleId, &Float: x, &Float: y, &Float: z)
```
* See the wiki for more information on this function.

---

```pawn
stock bool:SetDynamicVehiclePos(DynamicVehicle:dynamicVehicleId, Float:x, Float:y, Float:z)
```
* See the wiki for more information on this function.

---

```pawn
stock bool:GetDynamicVehicleZAngle(DynamicVehicle:dynamicVehicleId, &Float:angle)
```
* See the wiki for more information on this function.

---

```pawn
stock bool:GetDynamicVehicleRotationQuat(DynamicVehicle:dynamicVehicleId, &Float:w, &Float:x, &Float:y, &Float:z)
```
* ⚠️ I haven't really tested this function, but it should work fine, with some caveats:
  - If the vehicle isn't spawned, the maths is handled in code, and I didn't check if the maths is correct since I see no use for this function. `x` and `y` will always be `0.0` in this case.
  - If the vehicle is spawned, the native version is called on the vehicle. 
* See the wiki for more information on this function.

---

```pawn
stock Float:GetDynamicVehicleDistanceFromPoint(DynamicVehicle:dynamicVehicleId, Float:x, Float:y, Float:z)
```
* See the wiki for more information on this function.

---

```pawn
stock bool:SetDynamicVehicleZAngle(DynamicVehicle:dynamicVehicleId, Float:angle)
```
* See the wiki for more information on this function.

---

```pawn
stock bool:SetDynamicVehicleZAngle(DynamicVehicle:dynamicVehicleId, Float:angle)
```
* See the wiki for more information on this function.

---

```pawn
stock bool:SetDynamicVehicleParamsForPlayer(DynamicVehicle:dynamicVehicleId, playerid, objective = -1, doors = -1)
```
* ⚠️ Due to how samp handles this native, you cannot assume that doors/objective will be unlocked/unset once a player walks away. Every time the vehicle is streamed, this params are re-applied, thus making it possible for the param to keep applied. However, if player walks away and isn't in the area once this vehicle is re-streamed, the doors/objective will be removed.
* See the wiki for more information on this function.

---

```pawn
stock bool:SetDynamicVehicleParamsEx(DynamicVehicle:dynamicVehicleId, engine = -1, lights = -1, alarm = -1, doors = -1, bonnet = -1, boot = -1, objective = -1)
```
* See the wiki for more information on this function.

---

```pawn
stock bool:GetDynamicVehicleParamsEx(DynamicVehicle:dynamicVehicleId, &engine = -1, &lights = -1, &alarm = -1, &doors = -1, &bonnet = -1, &boot = -1, &objective = -1)
```
* See the wiki for more information on this function.

---

```pawn
stock bool:SetDynamicVehicleParamsSirenState(DynamicVehicle:dynamicVehicleId, sirenState)
```
* See the wiki for more information on this function.

---

```pawn
stock GetDynamicVehicleParamsSirenState(DynamicVehicle:dynamicVehicleId)
```
* See the wiki for more information on this function.

---

```pawn
stock bool:SetDynamicVehicleParamsCarDoors(DynamicVehicle:dynamicVehicleId, frontLeft = -1, frontRight = -1, rearLeft = -1, rearRight = -1)
```
* See the wiki for more information on this function.

---

```pawn
stock bool:GetDynamicVehicleParamsCarDoors(DynamicVehicle:dynamicVehicleId, &frontLeft = -1, &frontRight = -1, &rearLeft = -1, &rearRight = -1)
```
* See the wiki for more information on this function.

---

```pawn
stock bool:SetDynamicVehicleParamsCarWindows(DynamicVehicle:dynamicVehicleId, frontLeft = -1, frontRight = -1, rearLeft = -1, rearRight = -1)
```
* See the wiki for more information on this function.

---

```pawn
stock bool:GetDynamicVehicleParamsCarWindows(DynamicVehicle:dynamicVehicleId, &frontLeft = -1, &frontRight = -1, &rearLeft = -1, &rearRight = -1)
```
* See the wiki for more information on this function.

---

```pawn
stock bool:SetDynamicVehicleToRespawn(DynamicVehicle:dynamicVehicleId)
```
* See the wiki for more information on this function.

---

```pawn
stock LinkDynamicVehicleToInterior(DynamicVehicle:dynamicVehicleId, interiorId)
```
* See the wiki for more information on this function.

---

```pawn
stock bool:AddDynamicVehicleComponent(DynamicVehicle:dynamicVehicleId, componentId)
```
* See the wiki for more information on this function.

---

```pawn
stock bool:RemoveDynamicVehicleComponent(DynamicVehicle:dynamicVehicleId, componentId)
```
* See the wiki for more information on this function.

---

```pawn
stock ChangeDynamicVehicleColours(DynamicVehicle:dynamicVehicleId, colour1, colour2)
stock ChangeDynamicVehicleColor(DynamicVehicle:dynamicVehicleId, colour1, colour2)
```
* ⚠️ This function respects `MIXED_SPELLINGS` open.mp define.
* See the wiki for more information on this function.

---

```pawn
stock ChangeDynamicVehiclePaintjob(DynamicVehicle:dynamicVehicleId, paintjobid)
```
* See the wiki for more information on this function.

---

```pawn
stock SetDynamicVehicleHealth(DynamicVehicle:dynamicVehicleId, Float:health)
```
* See the wiki for more information on this function.

---

```pawn
stock GetDynamicVehicleHealth(DynamicVehicle:dynamicVehicleId, &Float:health)
```
* See the wiki for more information on this function.

---

```pawn
stock AttachTrailerToDynamicVehicle(trailerid, DynamicVehicle:dynamicVehicleId)
```
Use when you want to attach a normal vehicle as a trailer to a dynamic vehicle.
* ⚠️ When the dynamic vehicle despawns, the trailer de-attaches and **won't be re-attached** once it re-spawns.
* See the wiki for more information on this function.

---

```pawn
stock AttachDynamicTrailerToVehicle(DynamicVehicle:dynamicTrailerId, vehicleid)
```
Use when you want to attach a dynamic vehicle as a trailer to a normal vehicle.
* ⚠️ When the dynamic vehicle despawns, the trailer de-attaches and **won't be re-attached** once it re-spawns.
* See the wiki for more information on this function.

---

```pawn
stock AttachDynamicTrailerToDynamicVehicle(DynamicVehicle:dynamicTrailerId, DynamicVehicle:dynamicVehicleId)
```
Use when you want to attach a dynamic vehicle as a trailer to a dynamic vehicle.
* ⚠️ When the dynamic vehicle despawns, the trailer de-attaches and **won't be re-attached** once it re-spawns.
* See the wiki for more information on this function.

---

```pawn
stock bool:DettachTrailerFromDynamicVehicle(DynamicVehicle:dynamicVehicleId)
```
* See the wiki for more information on this function.

---

```pawn
stock bool:IsTrailerAttachedToDynamicVehicle(DynamicVehicle:dynamicVehicleId)
```
* See the wiki for more information on this function.

---

```pawn
stock GetDynamicVehicleTrailer(DynamicVehicle:dynamicVehicleId)
```
* See the wiki for more information on this function.

---

```pawn
stock SetDynamicVehicleNumberPlate(DynamicVehicle:dynamicVehicleId, const numberPlate[])
```
* See the wiki for more information on this function.

---

```pawn
stock GetDynamicVehicleModel(DynamicVehicle:dynamicVehicleId)
```
* See the wiki for more information on this function.

---

```pawn
stock GetDynamicVehicleComponentInSlot(DynamicVehicle:dynamicVehicleId, CARMODTYPE:slot)
```
* See the wiki for more information on this function.

---

```pawn
stock RepairDynamicVehicle(DynamicVehicle:dynamicVehicleId)
```
* See the wiki for more information on this function.

---

```pawn
stock bool:GetDynamicVehicleVelocity(DynamicVehicle:dynamicVehicleId, &Float:x, &Float:y, &Float:z)
```
* See the wiki for more information on this function.

---

```pawn
stock bool:SetDynamicVehicleVelocity(DynamicVehicle:dynamicVehicleId, Float:x, Float:y, Float:z)
```
* See the wiki for more information on this function.

---

```pawn
stock bool:SetDynamicVehicleAngularVelocity(DynamicVehicle:dynamicVehicleId, Float:x, Float:y, Float:z)
```
* ⚠️ This function can only be used on streamed in dynamic vehicles, since there's no getter for it.
* See the wiki for more information on this function.

---

```pawn
stock bool:GetDynamicVehicleDamageStatus(DynamicVehicle:dynamicVehicleId, &__TAG(VEHICLE_PANEL_STATUS):panelState, &__TAG(VEHICLE_DOOR_STATUS):doorsState, &__TAG(VEHICLE_LIGHT_STATUS):lightsState, &__TAG(VEHICLE_TYRE_STATUS):tyresState)
```
* See the wiki for more information on this function.

---

```pawn
stock bool:UpdateDynamicVehicleDamageStatus(DynamicVehicle:dynamicVehicleId, __TAG(VEHICLE_PANEL_STATUS):panelState, __TAG(VEHICLE_DOOR_STATUS):doorsState, __TAG(VEHICLE_LIGHT_STATUS):lightsState, __TAG(VEHICLE_TYRE_STATUS):tyresState)
```
* See the wiki for more information on this function.

---

```pawn
stock bool:SetDynamicVehicleVirtualWorld(DynamicVehicle:dynamicVehicleId, virtualWorld)
```
* See the wiki for more information on this function.

---

```pawn
stock GetDynamicVehicleVirtualWorld(DynamicVehicle:dynamicVehicleId)
```
* See the wiki for more information on this function.

---

```pawn
stock bool:GetDynamicVehicleSirenState(DynamicVehicle:dynamicVehicleId)
```
* ⚠️ Only available if you have `GetVehicleSirenState` defined in your gamemode.
* See the wiki for more information on this function.

---

```pawn
stock __TAG(LANDING_GEAR_STATE):GetDynamicVehicleLandingGearState(DynamicVehicle:dynamicVehicleId)
```
* ⚠️ Only available if you have `GetVehicleLandingGearState` defined in your gamemode.
* See the wiki for more information on this function.

---

```pawn
stock bool:GetDynamicVehicleSpawnInfo(DynamicVehicle:dynamicVehicleId, &Float:spawnX, &Float:spawnY, &Float:spawnZ, &Float:angle, &colour1 = 0, &colour2 = 0)
```
* See the wiki for more information on this function.

---

```pawn
stock bool:SetDynamicVehicleSpawnInfo(DynamicVehicle:dynamicVehicleId, modelid, Float:spawnX, Float:spawnY, Float:spawnZ, Float:angle, colour1, colour2, respawnDelay = -2, interior = -2)
```
* See the wiki for more information on this function.

---

```pawn
stock bool:GetDynamicVehicleColor(DynamicVehicle:dynamicVehicleId, &color1, &color2)
stock bool:GetDynamicVehicleColurs(DynamicVehicle:dynamicVehicleId, &color1, &color2)
```
* See the wiki for more information on this function.

---

```pawn
stock GetDynamicVehiclePaintJob(DynamicVehicle:dynamicVehicleId)
```
* See the wiki for more information on this function.

---

```pawn
stock GetDynamicVehicleInterior(DynamicVehicle:dynamicVehicleId)
```
* See the wiki for more information on this function.

---

```pawn
stock GetDynamicVehicleNumberPlate(DynamicVehicle:dynamicVehicleId, plate[], len = sizeof(plate))
```
* ⚠️ Only available if you have `GetDynamicVehicleNumberPlate` defined in your gamemode.
* See the wiki for more information on this function.

---

```pawn
stock bool:SetDynamicVehicleRespawnDelay(DynamicVehicle:dynamicVehicleId, respawnDelay)
```
* See the wiki for more information on this function.

---

```pawn
stock GetDynamicVehicleRespawnDelay(DynamicVehicle:dynamicVehicleId)
```
* See the wiki for more information on this function.

---

```pawn
stock GetDynamicVehicleOccupiedTick(DynamicVehicle:dynamicVehicleId)
```
* ⚠️ Only available if you have `GetVehicleOccupiedTick` defined in your gamemode.
* See the wiki for more information on this function.

---

```pawn
stock SetDynamicVehicleOccupiedTick(DynamicVehicle:dynamicVehicleId, ticks)
```
* ⚠️ Only available if you have `SetVehicleOccupiedTick` defined in your gamemode.
* See the wiki for more information on this function.

---

```pawn
stock GetDynamicVehicleRespawnTick(DynamicVehicle:dynamicVehicleId)
```
* ⚠️ Only available if you have `GetVehicleRespawnTick` defined in your gamemode.
* See the wiki for more information on this function.

---

```pawn
stock GetDynamicVehicleLastDriver(DynamicVehicle:dynamicVehicleId)
```
* ⚠️ Only available if you have `GetVehicleLastDriver` defined in your gamemode.
* See the wiki for more information on this function.

---

```pawn
stock GetDynamicVehicleDriver(DynamicVehicle:dynamicVehicleId)
```
* See the wiki for more information on this function.

---

```pawn
stock GetDynamicVehicleCab(DynamicVehicle:dynamicVehicleId)
```
* See the wiki for more information on this function.

---

```pawn
stock bool:HasDynamicVehicleBeenOccupied(DynamicVehicle:dynamicVehicleId)
```
* ⚠️ Only available if you have `HasVehicleBeenOccupied` defined in your gamemode.
* See the wiki for more information on this function.

---

```pawn
stock bool:IsDynamicVehicleOccupied(DynamicVehicle:dynamicVehicleId)
```
* See the wiki for more information on this function.

---

```pawn
stock bool:IsDynamicVehicleDead(DynamicVehicle:dynamicVehicleId)
```
* See the wiki for more information on this function.

---

```pawn
stock bool:ToggleDynamicVehicleSirenEnabled(DynamicVehicle:dynamicVehicleId, bool:enabled)
```
* See the wiki for more information on this function.

---

```pawn
stock bool:GetDynamicVehicleMatrix(DynamicVehicle:dynamicVehicleId, &Float:rightX, &Float:rightY, &Float:rightZ, &Float:upX, &Float:upY, &Float:upZ, &Float:atX, &Float:atY, &Float:atZ)
```
* ⚠️ I haven't really tested this function, but it should work fine, with some caveats:
  - If the vehicle isn't spawned, the maths is handled in code, and I didn't check if the maths is correct since I see no use for this function. Vehicle will always be upside if this is the case.
* See the wiki for more information on this function.

---

```pawn
stock GetDynamicVehicleOccupant(DynamicVehicle:dynamicVehicleId, seatid)
```
* See the wiki for more information on this function.

---

```pawn
stock CountDynamicVehicleOccupants(DynamicVehicle:dynamicVehicleId)
```
* See the wiki for more information on this function.

---

```pawn
stock GetDynamicVehicleTower(DynamicVehicle:dynamicVehicleId)
```
* See the wiki for more information on this function.

---

```pawn
stock PutPlayerInDynamicVehicle(DynamicVehicle:dynamicVehicleId, playerid, seatid)
```
* See the wiki for more information on this function.

---

```pawn
stock bool:IsPlayerInDynamicVehicle(DynamicVehicle:dynamicVehicleId, playerid)
```
* See the wiki for more information on this function.

# Callbacks
### ⚠️ When the dynamic vehicle version of a callback is called, the normal vehicle version is NOT called.

```pawn
forward OnDynamicVehicleSirenStateChange(playerid, DynamicVehicle:dynamicVehicleId, newstate);
```
* See the wiki for more information on this callback.

---

```pawn
/* isInternalStream is true only when the vehicle is streamed by the include itself (CreateVehicle inside the include). /
/ when isInternalStream is true, forplayerid will be INVALID_PLAYER_ID */
forward OnDynamicVehicleStreamIn(DynamicVehicle:dynamicVehicleId, forplayerid, bool:isInternalStream);
```
* `isInternalStream` is true only when the vehicle is streamed by the include.
* When `isInternalStream` is true, forplayerid will be INVALID_PLAYER_ID
* See the wiki for more information on this callback.

---

```pawn
forward OnDynamicVehicleStreamOut(DynamicVehicle:dynamicVehicleId, forplayerid, bool:isInternalStream);
```
* `isInternalStream` is true only when the vehicle is streamed by the include.
* When `isInternalStream` is true, forplayerid will be INVALID_PLAYER_ID
* See the wiki for more information on this callback.

---

```pawn
forward OnPlayerEnterDynamicVehicle(DynamicVehicle:dynamicVehicleId, playerid, ispassenger);
```
* See the wiki for more information on this callback.

---

```pawn
forward OnPlayerExitDynamicVehicle(DynamicVehicle:dynamicVehicleId, playerid);
```
* See the wiki for more information on this callback.

---

```pawn
forward OnDynamicVehicleSpawn(DynamicVehicle:dynamicVehicleId);
```
* Only called when dynamic vehicle is respawned.
* See the wiki for more information on this callback.

---

```pawn
forward OnDynamicVehicleDeath(DynamicVehicle:dynamicVehicleId, killerid);
```
* See the wiki for more information on this callback.

---

```pawn
forward OnDynamicVehicleMod(DynamicVehicle:dynamicVehicleId, playerid, componentid);
```
* See the wiki for more information on this callback.

---

```pawn
forward OnDynamicVehiclePaintjob(DynamicVehicle:dynamicVehicleId, playerid, paintjobid);
```
* See the wiki for more information on this callback.

---

```pawn
forward OnDynamicVehicleRespray(DynamicVehicle:dynamicVehicleId, playerid, color1, color2);
```
* See the wiki for more information on this callback.

---

```pawn
forward OnDynamicVehicleDamageStatusUpdate(DynamicVehicle:dynamicVehicleId, playerid);
```
* See the wiki for more information on this callback.

---

```pawn
forward OnDynamicVehicleUnoccupiedUpdate(DynamicVehicle:dynamicVehicleId, playerid, passenger_seat, Float:new_x, Float:new_y, Float:new_z, Float:vel_x, Float:vel_y, Float:vel_z);
```
* See the wiki for more information on this callback.

---

```pawn
public OnPlayerWeaponShot(playerid, __TAG(WEAPON):weaponid, __TAG(BULLET_HIT_TYPE):hittype, hitid, Float:fX, Float:fY, Float:fZ)
```
* ⚠️ While this callback isn't a dynamic vehicle specific one, the hittype has been modified to also trigger with type `BULLET_HIT_TYPE_DYNAMIC_VEHICLE`(`5`) 
* See the wiki for more information on this callback.
