#include <math.h>
#include <maya/MIOStream.h>
#include <maya/MSimple.h>
#include <maya/MPoint.h>
#include <maya/MPointArray.h>
#include <maya/MDoubleArray.h>
#include <maya/MFnNurbsCurve.h>
#include <maya\MFnMesh.h>
DeclareSimpleCommand(doHelix, "Autodesk - Example", "2012");

MObject createCustomMesh(MStatus* errorStatus, MPoint prevPos, MPoint nextPos)
{
	float sqrtTwo = sqrt(2);
	MFnMesh newMesh;

	MPointArray vertices;
	MFloatVector center = (nextPos + prevPos) / 2;
	MFloatVector dir = (nextPos - prevPos);
	dir.normalize();
	MFloatVector perpendicularDirHorizontal = MFloatVector(dir.z, dir.y, dir.x);
	MFloatVector perpendicularDirVertical = perpendicularDirHorizontal^dir;
	float distBetween = prevPos.distanceTo(nextPos) / 2;

	MFloatVector dirTemp;
	dirTemp = dir + perpendicularDirHorizontal;
	dirTemp.normalize();
	dirTemp *= distBetween * sqrtTwo;

	vertices.append(dirTemp + center + perpendicularDirVertical * (distBetween));    // 0

	dirTemp = dir + perpendicularDirHorizontal;
	dirTemp.normalize();
	dirTemp *= distBetween * sqrtTwo;

	vertices.append(dirTemp + center - perpendicularDirVertical * (distBetween));    // 1

	dirTemp = -dir + perpendicularDirHorizontal;
	dirTemp.normalize();
	dirTemp *= distBetween * sqrtTwo;

	vertices.append(dirTemp + center - perpendicularDirVertical * (distBetween));    // 2

	dirTemp = -dir + perpendicularDirHorizontal;
	dirTemp.normalize();
	dirTemp *= distBetween * sqrtTwo;

	vertices.append(dirTemp + center + perpendicularDirVertical * (distBetween));    // 3

	dirTemp = dir - perpendicularDirHorizontal;
	dirTemp.normalize();
	dirTemp *= distBetween * sqrtTwo;

	vertices.append(dirTemp + center - perpendicularDirVertical * (distBetween));    // 4

	dirTemp = dir - perpendicularDirHorizontal;
	dirTemp.normalize();
	dirTemp *= distBetween * sqrtTwo;

	vertices.append(dirTemp + center + perpendicularDirVertical * (distBetween));    // 5

	dirTemp = -dir - perpendicularDirHorizontal;
	dirTemp.normalize();
	dirTemp *= distBetween * sqrtTwo;

	vertices.append(dirTemp + center + perpendicularDirVertical * (distBetween));    // 6

	dirTemp = -dir - perpendicularDirHorizontal;
	dirTemp.normalize();
	dirTemp *= distBetween * sqrtTwo;

	vertices.append(dirTemp + center - perpendicularDirVertical * (distBetween));    // 7

	// Number of vertices per polygon
	MIntArray polygonCounts;
	polygonCounts.append(4);
	polygonCounts.append(4);
	polygonCounts.append(4);
	polygonCounts.append(4);
	polygonCounts.append(4);
	polygonCounts.append(4);

	// How each group of N vertices per polygon is connected.
	//
	// Because for each vertex I have specified the list index number, I can use the
	// same number to uniquely identify each face of the polygon to actually build
	// the connection between each vertex.
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
MObject createSplineMesh(MStatus* errorStatus, const MFnNurbsCurve& curveRef)
{
	MFnMesh newMesh;
	MPointArray vertices;
	MIntArray polygonCounts;
	MIntArray polygonConnects;
	MPointArray verticesTemp;
	curveRef.getCVs(verticesTemp);
	for (size_t i = 0; i < verticesTemp.length() - 1; i++)
	{
		createCustomMesh(errorStatus, verticesTemp[i], verticesTemp[i+1]);
	}

	return newMesh.create(8, 6, vertices, polygonCounts, polygonConnects, &errorStatus);
}

MStatus doHelix::doIt(const MArgList&)
{
	MStatus stat;
	const unsigned deg = 3; // Curve Degree
	const unsigned ncvs = 20; // Number of CVs
	const unsigned spans = ncvs - deg; // Number of spans
	const unsigned nknots = spans + 2 * deg - 1; // Number of knots
	double radius = 4.0; // Helix radius
	double pitch = 0.5; // Helix pitch
	unsigned i;
	MPointArray controlVertices;
	MDoubleArray knotSequences;
	// Set up cvs and knots for the helix
	//
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

	MObject newMesh = createSplineMesh(&stat, curveFn);
	if (MS::kSuccess != stat)
		cout << "Error creating cube.\n";

	return stat;
}
