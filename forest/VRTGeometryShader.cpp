#include "VRTGeometryShader.h"
#include <osg/AlphaFunc>
#include <osg/Billboard>
#include <osg/BlendFunc>
#include <osg/Depth>
#include <osg/Geode>
#include <osg/Material>
#include <osg/Math>
#include <osg/MatrixTransform>
#include <osg/PolygonOffset>
#include <osg/Projection>
#include <osg/ShapeDrawable>
#include <osg/Texture2D>
#include <osg/TextureBuffer>
#include <osg/LOD>
#include <osg/Image>
#include "VegetationCell.h"


namespace osgVegetation
{
	osg::Node* VRTGeometryShader::create(Cell* cell, osg::StateSet* dstate)
	{
		bool needGroup = !(cell->_cells.empty());
		bool needTrees = !(cell->_trees.empty());

		osg::Geode* geode = 0;
		osg::Group* group = 0;

		if (needTrees)
		{
			geode = new osg::Geode;
			geode->setStateSet(dstate);

			osg::Geometry* geometry = new osg::Geometry;
			geode->addDrawable(geometry);

			osg::Vec3Array* v = new osg::Vec3Array;

			for(TreeList::iterator itr=cell->_trees.begin();
				itr!=cell->_trees.end();
				++itr)
			{
				Tree& tree = **itr;
				v->push_back(tree._position);
				v->push_back(osg::Vec3(tree._height,(double)random(0.75f,1.15f),(double)random(1.0f,1.250f)));
				v->push_back(osg::Vec3(tree._width,tree._type,0));
			}
			geometry->setVertexArray( v );
			geometry->addPrimitiveSet( new osg::DrawArrays( GL_TRIANGLES, 0, v->size() ) );

			osg::StateSet* sset = geode->getOrCreateStateSet();
			sset->setMode( GL_LIGHTING, osg::StateAttribute::OFF );
			sset->setAttribute( createGeometryShader() );

			//osg::Uniform* baseTextureSampler = new osg::Uniform("baseTexture",0);
			//osg::Uniform* baseTextureSampler = new osg::Uniform(osg::Uniform::SAMPLER_2D_ARRAY, "baseTexture", 0);
			//sset->addUniform(baseTextureSampler);

		}

		if (needGroup)
		{
			group = new osg::Group;
			//group->setCenter(cell->_bb.center());
			for(Cell::CellList::iterator itr=cell->_cells.begin();
				itr!=cell->_cells.end();
				++itr)
			{
				group->addChild(create(itr->get(),dstate));
				
			}

			if (geode) group->addChild(geode);

		}
		if (group) return group;
		else return geode;
	}

	osg::Program* VRTGeometryShader::createGeometryShader()
	{
		static const char* vertSource = {
			"#version 120\n"
			"#extension GL_EXT_geometry_shader4 : enable\n"
			"varying vec2 texcoord;\n"
			"void main(void)\n"
			"{\n"
			"    gl_Position = gl_Vertex;\n"
			"    texcoord = gl_MultiTexCoord0.st;\n"
			"}\n"
		};

		static const char* geomSource = {
			"#version 120\n"
			"#extension GL_EXT_geometry_shader4 : enable\n"
			"varying vec2 texcoord;\n"
			"varying float intensity; \n"
			"varying float red_intensity; \n"
			"varying float veg_type; \n"
			"void main(void)\n"
			"{\n"
			"    vec4 v = gl_PositionIn[0];\n"
			"    vec4 info = gl_PositionIn[1];\n"
			"    vec4 info2 = gl_PositionIn[2];\n"
			"    intensity = info.y;\n"
			"    red_intensity = info.z;\n"
			"    veg_type = info2.y;\n"
			"\n"
			"    float h = info.x;\n"
			"    float w = info2.x;\n"
			"    vec4 e;\n"
			"    e = v + vec4(-w,0.0,0.0,0.0);  gl_Position = gl_ModelViewProjectionMatrix * e; texcoord = vec2(0.0,0.0); EmitVertex();\n"
			"    e = v + vec4(w,0.0,0.0,0.0);  gl_Position = gl_ModelViewProjectionMatrix * e;  texcoord = vec2(1.0,0.0); EmitVertex();\n"
			"    e = v + vec4(-w,0.0,h,0.0);  gl_Position = gl_ModelViewProjectionMatrix * e;  texcoord = vec2(0.0,1.0); EmitVertex();\n"
			"    e = v + vec4(w,0.0,h,0.0);  gl_Position = gl_ModelViewProjectionMatrix * e;  texcoord = vec2(1.0,1.0); EmitVertex();\n"
			"    EndPrimitive();\n"
			"    e = v + vec4(0.0,-w,0.0,0.0);  gl_Position = gl_ModelViewProjectionMatrix * e; texcoord = vec2(0.0,0.0); EmitVertex();\n"
			"    e = v + vec4(0.0,w,0.0,0.0);  gl_Position = gl_ModelViewProjectionMatrix * e;  texcoord = vec2(1.0,0.0); EmitVertex();\n"
			"    e = v + vec4(0.0,-w,h,0.0);  gl_Position = gl_ModelViewProjectionMatrix * e;  texcoord = vec2(0.0,1.0); EmitVertex();\n"
			"    e = v + vec4(0.0,w,h,0.0);  gl_Position = gl_ModelViewProjectionMatrix * e;  texcoord = vec2(1.0,1.0); EmitVertex();\n"
			"    EndPrimitive();\n"
			"}\n"
		};


		static const char* fragSource = {
			"#version 120\n"
			"#extension GL_EXT_gpu_shader4 : enable\n"
			"#extension GL_EXT_texture_array : enable\n"
			"uniform sampler2DArray baseTexture; \n"
			"varying vec2 texcoord; \n"
			"varying float intensity; \n"
			"varying float red_intensity; \n"
			"varying float veg_type; \n"
			"\n"
			"void main(void) \n"
			"{ \n"
			"   vec4 finalColor = texture2DArray( baseTexture, vec3(texcoord, veg_type)); \n"
			"   vec4 color = finalColor * intensity;\n"
			"   color.w = finalColor.w;\n"
			"   color.x *= red_intensity;\n"
			"   gl_FragColor = color;\n"
			"}\n"
		};

		osg::Program* pgm = new osg::Program;
		pgm->setName( "osgshader2 demo" );

		pgm->addShader( new osg::Shader( osg::Shader::VERTEX,   vertSource ) );
		pgm->addShader( new osg::Shader( osg::Shader::FRAGMENT, fragSource ) );

		pgm->addShader( new osg::Shader( osg::Shader::GEOMETRY, geomSource ) );
		pgm->setParameter( GL_GEOMETRY_VERTICES_OUT_EXT, 8 );
		pgm->setParameter( GL_GEOMETRY_INPUT_TYPE_EXT, GL_LINES );
		pgm->setParameter( GL_GEOMETRY_OUTPUT_TYPE_EXT, GL_TRIANGLE_STRIP);

		return pgm;
	}
}