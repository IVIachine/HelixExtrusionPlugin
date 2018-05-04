#include <math.h>
#include <maya/MIOStream.h>
#include <maya/MSimple.h>
#include <maya/MPoint.h>
#include <maya/MPointArray.h>
#include <maya/MDoubleArray.h>
#include <maya/MFnNurbsCurve.h>
#include <maya\MFnMesh.h>

DeclareSimpleCommand(HelixExtrude, "Autodesk", "2017");

MObject createCustomMesh(MStatus* errorStatus, MPoint prevPos, MPoint nextPos, double currentWidth,
	MPoint &lastFaceOne, MPoint &lastFaceTwo, MPoint &lastFaceThree, MPoint &lastFaceFour)
{
	double sqrtTwo = sqrt(2);
	MFnMesh newMesh;

	MPointArray vertices;
	MFloatVector center = (nextPos + prevPos) / 2;
	MFloatVector dir = (nextPos - prevPos);
	dir.normalize();
	MFloatVector perpendicularDirHorizontal = MFloatVector(-dir.z, dir.y, dir.x);
	MFloatVector perpendicularDirVertical = perpendicularDirHorizontal^dir;
	double distBetween = (prevPos.distanceTo(nextPos) / 2) * currentWidth;

#pragma region setVertices

	//set vertices based on direction vector of two helix samples

	//find point at corner of box based on direction vectors
	MFloatVector dirTemp;
	dirTemp = dir + perpendicularDirHorizontal;
	dirTemp.normalize();
	dirTemp *= distBetween * sqrtTwo;


	vertices.append(dirTemp + center + perpendicularDirVertical * (distBetween));  // 0

	//do the same for other vertices

	dirTemp = dir + perpendicularDirHorizontal;
	dirTemp.normalize();
	dirTemp *= distBetween * sqrtTwo;

	vertices.append(dirTemp + center - perpendicularDirVertical * (distBetween));  // 1

	if (lastFaceOne == MPoint())
	{
		dirTemp = -dir + perpendicularDirHorizontal;
		dirTemp.normalize();
		dirTemp *= distBetween * sqrtTwo;

		vertices.append(dirTemp + center - perpendicularDirVertical * (distBetween));  // 2

		dirTemp = -dir + perpendicularDirHorizontal;
		dirTemp.normalize();
		dirTemp *= distBetween * sqrtTwo;

		vertices.append(dirTemp + center + perpendicularDirVertical * (distBetween));  // 3
	}
	else
	{
		vertices.append(lastFaceOne);
		vertices.append(lastFaceTwo);
	}

	dirTemp = dir - perpendicularDirHorizontal;
	dirTemp.normalize();
	dirTemp *= distBetween * sqrtTwo;

	vertices.append(dirTemp + center - perpendicularDirVertical * (distBetween));  // 4

	dirTemp = dir - perpendicularDirHorizontal;
	dirTemp.normalize();
	dirTemp *= distBetween * sqrtTwo;

	vertices.append(dirTemp + center + perpendicularDirVertical * (distBetween));  // 5


	if (lastFaceOne == MPoint())
	{
		dirTemp = -dir - perpendicularDirHorizontal;
		dirTemp.normalize();
		dirTemp *= distBetween * sqrtTwo;

		vertices.append(dirTemp + center + perpendicularDirVertical * (distBetween));  // 6

		dirTemp = -dir - perpendicularDirHorizontal;
		dirTemp.normalize();
		dirTemp *= distBetween * sqrtTwo;

		vertices.append(dirTemp + center - perpendicularDirVertical * (distBetween));  // 7
	}
	else
	{
		vertices.append(lastFaceThree);
		vertices.append(lastFaceFour);
	}

#pragma endregion

	//set prev faces to be called in next customMesh call
	lastFaceOne = vertices[1];
	lastFaceTwo = vertices[0];
	lastFaceThree = vertices[5];
	lastFaceFour = vertices[4];

	// Number of vertices per polygon
	MIntArray polygonCounts;
	polygonCounts.append(4);
	polygonCounts.append(4);
	polygonCounts.append(4);
	polygonCounts.append(4);
	polygonCounts.append(4);
	polygonCounts.append(4);

	// How each group of N vertices per polygon is connected.
	MIntArray polygonConnects;

	// First polygon
	polygonConnects.append(0);
	polygonConnects.append(1);
	polygonConnects.append(2);
	polygonConnects.append(3);

	// Second polygon
	polygonConnects.append(0);
	polygonConnects.append(5);
	polygonConnects.append(4);
	polygonConnects.append(1);

	// Third polygon
	polygonConnects.append(5);
	polygonConnects.append(6);
	polygonConnects.append(7);
	polygonConnects.append(4);

	// Fourth polygon
	polygonConnects.append(6);
	polygonConnects.append(3);
	polygonConnects.append(2);
	polygonConnects.append(7);

	// Fifth polygon
	polygonConnects.append(0);
	polygonConnects.append(3);
	polygonConnects.append(6);
	polygonConnects.append(5);

	// Sixth polygon
	polygonConnects.append(1);
	polygonConnects.append(4);
	polygonConnects.append(7);
	polygonConnects.append(2);

	return newMesh.create(8, 6, vertices, polygonCounts, polygonConnects, &errorStatus);
}

//CITE: https://stackoverflow.com/questions/42286385/how-to-create-a-polygon-in-maya-api-c
void createSplineMesh(MStatus* errorStatus, const MFnNurbsCurve& curveRef, double widthStart, double widthDecrease)
{
	//mesh data
	MFnMesh newMesh;
	MPointArray vertices;
	MIntArray polygonCounts;
	MIntArray polygonConnects;
	MPointArray verticesTemp;

	//temp points
	MPoint faceOne, faceTwo, faceThree, faceFour;
	curveRef.getCVs(verticesTemp);
	double width = widthStart;

	//loop through each point on the curve, pass in current and next point
	for (unsigned int i = 0; i < verticesTemp.length() - 1; i++)
	{
		createCustomMesh(errorStatus, verticesTemp[i], verticesTemp[i + 1], width, faceOne, faceTwo, faceThree, faceFour);
		width -= widthDecrease;
		width = width < 0 ? 0 : width;
	}
}

MStatus HelixExtrude::doIt(const MArgList& args)
{
	//cite helix generation: http://docs.autodesk.com/MAYAUL/2014/ENU/Maya-API-Documentation/index.html?url=files/GUID-A343C962-AEE1-4113-9DEC-1A88CF818AE9.htm,topicNumber=d30e7611
	//default args
	MStatus stat;
	const unsigned	deg = 3;			// Curve Degree
	const unsigned	ncvs = 20;			// Number of CVs
	const unsigned	spans = ncvs - deg;	// Number of spans
	const unsigned	nknots = spans + 2 * deg - 1;// Number of knots
	double	radius = 4.0;			// Helix radius
	double	pitch = 0.5;			// Helix pitch
	double startWidth = .5f;
	double decreaseWidth = .025f;
	unsigned i;

	// Parse the arguments.
	for (i = 0; i < args.length(); i++)
	{
		if (MString("-p") == args.asString(i, &stat)
			&& MS::kSuccess == stat)
		{
			double tmp = args.asDouble(++i, &stat);
			if (MS::kSuccess == stat)
				pitch = tmp;
		}
		else if (MString("-r") == args.asString(i, &stat)
			&& MS::kSuccess == stat)
		{
			double tmp = args.asDouble(++i, &stat);
			if (MS::kSuccess == stat)
				radius = tmp;
		}
		else if (MString("-sw") == args.asString(i, &stat)
			&& MS::kSuccess == stat)
		{
			double tmp = args.asDouble(++i, &stat);
			startWidth = tmp;
		}
		else if (MString("-dw") == args.asString(i, &stat)
			&& MS::kSuccess == stat)
		{
			double tmp = args.asDouble(++i, &stat);
			decreaseWidth = tmp;
		}
	}

	MPointArray controlVertices;
	MDoubleArray knotSequences;

	// Set up cvs and knots for the helix
	for (i = 0; i < ncvs; i++)
		controlVertices.append(MPoint(radius * cos((double)i),
			pitch * (double)i, radius * sin((double)i)));
	for (i = 0; i < nknots; i++)
		knotSequences.append((double)i);

	// Now create the curve
	MFnNurbsCurve curveFn;
	MObject curve = curveFn.create(controlVertices,
		knotSequences, deg, MFnNurbsCurve::kOpen, false, false, MObject::kNullObj, &stat);

	if (MS::kSuccess != stat)
		cout << "Error creating curve.\n";

	//take the curve and create a mesh from it
	createSplineMesh(&stat, curveFn, startWidth, decreaseWidth);

	if (MS::kSuccess != stat)
		cout << "Error with custom extrude.\n";

	return stat;
}
