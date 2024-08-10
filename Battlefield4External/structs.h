#pragma once

struct Vector3
{
	float x, y, z;

	Vector3 operator + (Vector3 v)
	{
		return { x + v.x, y + v.y, z + v.z };
	}
	Vector3 operator - (Vector3 v)
	{
		return { x - v.x, y - v.y, z - v.z };
	}
	Vector3 operator * (float v)
	{
		return { x * v, y * v, z * v };
	}
	Vector3 operator / (float v)
	{
		return { x / v, y / v, z / v };
	}
};

struct ViewAngles
{
	float pitch, yaw;
};