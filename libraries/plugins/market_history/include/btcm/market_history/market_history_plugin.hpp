#include <btcm/app/plugin.hpp>

#include <graphene/db/generic_index.hpp>

#include <boost/multi_index/composite_key.hpp>

//
// Plugins should #define their SPACE_ID's so plugins with
// conflicting SPACE_ID assignments can be compiled into the
// same binary (by simply re-assigning some of the conflicting #defined
// SPACE_ID's in a build script).
//
// Assignment of SPACE_ID's cannot be done at run-time because
// various template automagic depends on them being known at compile
// time.
//
#ifndef MARKET_HISTORY_SPACE_ID
#define MARKET_HISTORY_SPACE_ID 7
#endif

#ifndef MARKET_HISTORY_PLUGIN_NAME
#define MARKET_HISTORY_PLUGIN_NAME "market_history"
#endif


namespace btcm { namespace market_history {

using namespace chain;
using namespace graphene::db;

namespace detail
{
   class market_history_plugin_impl;
}

class market_history_plugin : public btcm::app::plugin
{
   public:
      market_history_plugin();
      virtual ~market_history_plugin();

      virtual std::string plugin_name()const override { return MARKET_HISTORY_PLUGIN_NAME; }
      virtual void plugin_set_program_options(
         boost::program_options::options_description& cli,
         boost::program_options::options_description& cfg ) override;
      virtual void plugin_initialize( const boost::program_options::variables_map& options ) override;
      virtual void plugin_startup() override;

      flat_set< uint32_t > get_tracked_buckets() const;
      uint32_t get_max_history_per_bucket() const;

   private:
      friend class detail::market_history_plugin_impl;
      std::unique_ptr< detail::market_history_plugin_impl > _my;
};

struct bucket_object : public abstract_object< bucket_object >
{
   static const uint8_t space_id = MARKET_HISTORY_SPACE_ID;
   static const uint8_t type_id = 1;

   price high()const { return asset( high_mbd, XUSD_SYMBOL ) / asset( high_btcm, BTCM_SYMBOL ); }
   price low()const { return asset( low_mbd, XUSD_SYMBOL ) / asset( low_btcm, BTCM_SYMBOL ); }

   fc::time_point_sec   open;
   uint32_t             seconds = 0;
   share_type           high_btcm;
   share_type           high_mbd;
   share_type           low_btcm;
   share_type           low_mbd;
   share_type           open_btcm;
   share_type           open_mbd;
   share_type           close_btcm;
   share_type           close_mbd;
   share_type           btcm_volume;
   share_type           mbd_volume;
};

struct order_history_object : public abstract_object< order_history_object >
{
   fc::time_point_sec   time;
   fill_order_operation op;
};

//struct by_id;
struct by_bucket;
typedef multi_index_container<
   bucket_object,
   indexed_by<
      hashed_unique< tag< by_id >, member< object, object_id_type, &object::id > >,
      ordered_unique< tag< by_bucket >,
         composite_key< bucket_object,
            member< bucket_object, uint32_t, &bucket_object::seconds >,
            member< bucket_object, fc::time_point_sec, &bucket_object::open >
         >,
         composite_key_compare< std::less< uint32_t >, std::less< fc::time_point_sec > >
      >
   >
> bucket_object_multi_index_type;

struct by_time;
typedef multi_index_container<
   order_history_object,
   indexed_by<
      hashed_unique< tag< by_id >, member< object, object_id_type, &object::id > >,
      ordered_non_unique< tag< by_time >, member< order_history_object, time_point_sec, &order_history_object::time > >
   >
> order_history_multi_index_type;

typedef generic_index< bucket_object, bucket_object_multi_index_type >        bucket_index;
typedef generic_index< order_history_object, order_history_multi_index_type > order_history_index;

} } // btcm::market_history

FC_REFLECT_DERIVED( btcm::market_history::bucket_object, (graphene::db::object),
                     (open)(seconds)
                     (high_btcm)(high_mbd)
                     (low_btcm)(low_mbd)
                     (open_btcm)(open_mbd)
                     (close_btcm)(close_mbd)
                     (btcm_volume)(mbd_volume) )

FC_REFLECT_DERIVED( btcm::market_history::order_history_object, (graphene::db::object),
                     (time)
                     (op) )
