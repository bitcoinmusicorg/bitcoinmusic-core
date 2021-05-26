#pragma once
#include <fstream>
#include <btcm/chain/protocol/block.hpp>

namespace btcm { namespace chain {
   struct index_entry;

   class block_database
   {
      public:
         void open( const fc::path& dbdir );
         bool is_open()const;
         void flush();
         void close();

         void store( const block_id_type& id, const signed_block& b );
         void remove( const block_id_type& id );

         bool                   contains( const block_id_type& id )const;
         block_id_type          fetch_block_id( uint32_t block_num )const;
         optional<signed_block> fetch_optional( const block_id_type& id )const;
         optional<signed_block> fetch_by_number( uint32_t block_num )const;
         optional<signed_block> last()const;
         optional<block_id_type> last_id()const;
      private:
         optional<index_entry> last_index_entry()const;
         fc::path _index_filename;
         mutable std::fstream _blocks;
         mutable std::fstream _block_num_to_pos;
   };
} }
