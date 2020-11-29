#include "animation_importer.h"
#include "halley/file_formats/halley-yamlcpp.h"
#include "halley/core/graphics/sprite/animation.h"
#include "halley/support/exception.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/tools/file/filesystem.h"

using namespace Halley;

void AnimationImporter::import( const ImportingAsset& asset, IAssetCollector& collector )
{
    Animation animation;
    parseAnimation( animation, gsl::as_bytes( gsl::span< const Byte >( asset.inputFiles.at( 0 ).data ) ) );
    collector.output( animation.getName(), AssetType::Animation, Serializer::toBytes( animation ) );
}

void AnimationImporter::parseAnimation( Animation& animation, gsl::span< const gsl::byte > data )
{
    String strData( reinterpret_cast< const char* >( data.data() ), data.size() );
    YAML::Node root = YAML::Load( strData.cppStr() );

    animation.setName( root[ "name" ].as< std::string >() );

    if ( root[ "spriteSheet" ].IsDefined() )
    {
        animation.setSpriteSheetName( root[ "spriteSheet" ].as< std::string >() );
    }

    if ( root[ "material" ].IsDefined() )
    {
        animation.setMaterialName( root[ "material" ].as< std::string >() );
    }

    size_t nDirections = 0;
    if ( root[ "directions" ].IsDefined() )
    {
        for ( auto directionNode : root[ "directions" ] )
        {
            String name = directionNode[ "name" ].as< std::string >( "default" );
            String fileName = directionNode[ "fileName" ].as< std::string >( name );
            bool flip = directionNode[ "flip" ].as< bool >( false );
            animation.addDirection( AnimationDirection( name, fileName, flip, int( nDirections ) ) );
            nDirections++;
        }
    }
    else
    {
        animation.addDirection( AnimationDirection( "default", "default", false, 0 ) );
    }

    for ( auto sequenceNode : root[ "sequences" ] )
    {
        String name = sequenceNode[ "name" ].as< std::string >( "default" );
        float fps = sequenceNode[ "fps" ].as< float >( 0.0f );
        int frameDuration = lround( 1000.0 / fps );
        bool loop = sequenceNode[ "loop" ].as< bool >( true );
        bool noFlip = sequenceNode[ "noFlip" ].as< bool >( false );
        AnimationSequence sequence( name, loop, noFlip );
        String fileName = sequenceNode[ "fileName" ].as< std::string >();

        // Load frames
        size_t framesAdded = 0;
        if ( sequenceNode[ "frames" ].IsDefined() )
        {
            for ( const auto& frameNode : sequenceNode[ "frames" ] )
            {
                int duration = frameNode[ "duration" ].as< int >( 100 );
                String file_name = frameNode[ "image" ].as< std::string >( fileName );
                int frame_num = frameNode[ "frame" ].as< int >( 0 );
                sequence.addFrame( AnimationFrameDefinition( frame_num, duration, file_name ) );
                framesAdded++;
            }
        }

        // No frames listed,
        if ( framesAdded == 0 )
        {
            sequence.addFrame( AnimationFrameDefinition( 0, frameDuration, fileName ) );
        }

        animation.addSequence( sequence );
    }
}
