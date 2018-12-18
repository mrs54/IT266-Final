#include "g_local.h"
/*
=======================
Flamethrower
=======================
*/
void Fire_Think(edict_t *self)
{
	vec3_t dir;
	int damage;
	float	points;
	vec3_t	v;

	if (level.time > self->delay)
	{
		self->owner->Flames--;
		G_FreeEdict(self);
		return;
	}
	//if (self->owner->takedamage = DAMAGE_NO)
	//{
	//	self->owner->Flames--;
	//	G_FreeEdict (self);
	//		return;
	//	}
	if (!self->owner)
	{
		G_FreeEdict(self);
		return;
	}
	if (self->owner->waterlevel)
	{
		self->owner->Flames--;
		G_FreeEdict(self);
		return;
	}
	damage = rndnum(1, self->FlameDamage);
	VectorAdd(self->orb->mins, self->orb->maxs, v);
	VectorMA(self->orb->s.origin, 0.5, v, v);
	VectorSubtract(self->s.origin, v, v);
	points = damage - 0.5 * (VectorLength(v));
	VectorSubtract(self->owner->s.origin, self->s.origin, dir);

	if (self->PlasmaDelay < level.time)
	{
		T_Damage(self->owner, self, self->orb, dir, self->owner->s.origin, vec3_origin, damage, 0, DAMAGE_NO_KNOCKBACK, MOD_WF_FLAME);
		self->FlameDelay = level.time + 0.8;
	}
	VectorCopy(self->owner->s.origin, self->s.origin);
	self->nextthink = level.time + .2;
}

void burn_person(edict_t *target, edict_t *owner, int damage)
{
	edict_t	*flame;

	if (target->Flames > 1)//This number sets the allowed amount of flames per person
		return;
	target->Flames++;
	flame = G_Spawn();
	flame->movetype = MOVETYPE_NOCLIP;
	flame->clipmask = MASK_SHOT;
	flame->solid = SOLID_NOT;
	flame->s.effects |= EF_ANIM_ALLFAST | EF_BFG | EF_HYPERBLASTER;//|EF_GRENADE|EF_BLASTER;
	flame->velocity[0] = target->velocity[0];
	flame->velocity[1] = target->velocity[1];
	flame->velocity[2] = target->velocity[2];

	VectorClear(flame->mins);
	VectorClear(flame->maxs);
	flame->s.modelindex = gi.modelindex("sprites/fire.sp2");
	flame->owner = target;
	flame->orb = owner;
	flame->delay = level.time + 5;//8;//Jr Shorten it so it goes away faster
	flame->nextthink = level.time + .8;
	flame->FlameDelay = level.time + 0.8;
	flame->think = Fire_Think;
	flame->FlameDamage = damage + 2;//JR increased that
	flame->classname = "fire";
	flame->s.sound = gi.soundindex("weapons/bfg__l1a.wav");
	gi.linkentity(flame);

	VectorCopy(target->s.origin, flame->s.origin);
}
void flame_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{

	if (other == self->owner)
		return;

	// clean up laser entities

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict(self);
		return;
	}

	if (self->owner->client)
		PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT);
	T_Damage(other, self, self->owner, self->velocity, self->s.origin, plane->normal, 6, 0, 0, MOD_WF_FLAME);
	// core explosion - prevents firing it into the wall/floor
	if (other->health)
	{
		burn_person(other, self->owner, self->SniperDamage);
	}
	G_FreeEdict(self);


}

void fire_flamethrower(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, float damage_radius)
{
	edict_t	*flame;

	flame = G_Spawn();
	flame->wf_team = self->wf_team;
	VectorCopy(start, flame->s.origin);
	VectorCopy(dir, flame->movedir);
	vectoangles(dir, flame->s.angles);
	VectorScale(dir, speed, flame->velocity);
	flame->movetype = MOVETYPE_FLYMISSILE;
	flame->clipmask = MASK_SHOT;
	flame->solid = SOLID_BBOX;
	flame->s.effects |= EF_ANIM_ALLFAST | EF_BFG | EF_HYPERBLASTER;//EF_BLASTER|EF_GRENADE;
	VectorSet(flame->mins, -20, -20, -20);
	VectorSet(flame->maxs, 20, 20, 20);
	flame->s.modelindex = gi.modelindex("sprites/fire.sp2");
	flame->owner = self;
	flame->touch = flame_touch;
	flame->nextthink = level.time + 250 / speed;
	flame->think = G_FreeEdict;
	flame->radius_dmg = damage;
	flame->FlameDamage = damage;
	flame->dmg_radius = damage_radius;
	flame->classname = "flame";
	flame->s.sound = gi.soundindex("weapons/bfg__l1a.wav");


	if (self->client)
		check_dodge(self, flame->s.origin, dir, speed);

	gi.linkentity(flame);
}

void weapon_flamethrower_fire(edict_t *ent)
{
	vec3_t	offset, start;
	vec3_t	forward, right;
	int		damage = 5;
	float	damage_radius = 1000;
	if (!(ent->client->buttons & BUTTON_ATTACK))
	{
		ent->client->ps.gunframe = 33;
		return;
	}
	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_ROCKET | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);



	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (is_quad)
		damage *= 4;

	AngleVectors(ent->client->v_angle, forward, right, NULL);

	VectorSet(offset, 8, 8, ent->viewheight - 8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	fire_flamethrower(ent, start, forward, damage, 500, damage_radius);


	ent->client->ps.gunframe++;

	PlayerNoise(ent, start, PNOISE_WEAPON);

	ent->client->pers.inventory[ent->client->ammo_index] -= ent->client->pers.weapon->quantity;
}

void Weapon_FlameThrower(edict_t *ent)
{
	static int	pause_frames[] = { 39, 45, 50, 55, 0 };
	static int	fire_frames[] = { 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31, 0 };
	Weapon_Generic(ent, 8, 32, 55, 58, pause_frames, fire_frames, weapon_flamethrower_fire);
}
