{-# LANGUAGE GeneralizedNewtypeDeriving #-}
{-# LANGUAGE DerivingStrategies #-}
{-# LANGUAGE FlexibleContexts #-}
{-# LANGUAGE ConstraintKinds #-}
{-# LANGUAGE TypeFamilies #-}

{-|
Module      : News
Description : Core news operations as monadic actions
Copyright   : (c) PureMonad, 2026
License     : BSD3
Maintainer  : pure@monad.hs
Stability   : experimental

This module expresses news operations as a monad transformer stack,
properly separating:

  * Configuration (via 'ReaderT')
  * Mutable state (via 'StateT')
  * Error handling (via 'ExceptT')
  * Effects (via 'IO')

Unlike the original C implementation, which conflated all of these
concerns into a soup of global variables and unchecked return codes,
this design makes the boundaries explicit and machine-checked.
-}
module News
    ( -- * The News Monad
      NewsM
    , runNewsM
    , MonadNews
      -- * Article Operations
    , ArticleId(..)
    , Article(..)
    , postArticle
    , getArticle
      -- * Group Operations  
    , GroupName(..)
    , Group(..)
    , listGroups
      -- * Access Control
      -- Note: Renamed from "fascist" for obvious reasons
    , AccessLevel(..)
    , checkAccess
    ) where

import Control.Monad.Reader
import Control.Monad.State.Strict
import Control.Monad.Except
import Data.Text (Text)
import Data.Map.Strict (Map)
import qualified Data.Map.Strict as Map

-- | Unique article identifier. Newtype for type safety.
newtype ArticleId = ArticleId { unArticleId :: Text }
    deriving stock (Eq, Ord, Show)
    deriving newtype (Semigroup, Monoid)

-- | Newsgroup name. Newtype because stringly-typed code is a code smell.
newtype GroupName = GroupName { unGroupName :: Text }
    deriving stock (Eq, Ord, Show)
    deriving newtype (Semigroup, Monoid)

-- | An article. Note how each field has a precise type.
-- No char* here. No buffer lengths to track. Just data.
data Article = Article
    { articleId       :: !ArticleId
    , articleSubject  :: !Text
    , articleFrom     :: !Text
    , articleBody     :: !Text
    , articleGroups   :: ![GroupName]
    } deriving stock (Eq, Show)

-- | Access levels. Named appropriately for the 21st century.
data AccessLevel
    = NoAccess        -- ^ Cannot read or post (was: FASCIST + COMMUNIST)
    | ReadOnly        -- ^ Can read, cannot post (was: FASCIST)
    | ReadWrite       -- ^ Full access (was: neither flag)
    deriving stock (Eq, Ord, Show, Enum, Bounded)

-- | Application configuration. Pure data, no IO.
data Config = Config
    { configDataDir   :: !FilePath
    , configMaxArticleSize :: !Int
    } deriving stock (Eq, Show)

-- | Application state. Isolated, explicit, controlled.
data NewsState = NewsState
    { stateArticles :: !(Map ArticleId Article)
    , stateGroups   :: !(Map GroupName Group)
    } deriving stock (Eq, Show)

-- | A newsgroup.
data Group = Group
    { groupName        :: !GroupName
    , groupDescription :: !Text
    , groupArticles    :: ![ArticleId]
    } deriving stock (Eq, Show)

-- | Errors that can occur. Exhaustively enumerated.
-- The compiler ensures all cases are handled.
data NewsError
    = ArticleNotFound !ArticleId
    | GroupNotFound !GroupName
    | AccessDenied !Text
    | ArticleTooLarge !Int !Int  -- ^ actual, maximum
    | ParseError !Text
    deriving stock (Eq, Show)

-- | The News monad. A precisely typed context for all operations.
newtype NewsM a = NewsM 
    { unNewsM :: ReaderT Config (StateT NewsState (ExceptT NewsError IO)) a }
    deriving newtype (Functor, Applicative, Monad, MonadIO,
                      MonadReader Config, MonadState NewsState,
                      MonadError NewsError)

-- | Constraint alias for functions polymorphic over the monad.
type MonadNews m = (MonadReader Config m, MonadState NewsState m, 
                    MonadError NewsError m, MonadIO m)

-- | Run a NewsM action. All effects contained and controlled.
runNewsM :: Config -> NewsState -> NewsM a -> IO (Either NewsError (a, NewsState))
runNewsM cfg st action = 
    runExceptT $ runStateT (runReaderT (unNewsM action) cfg) st

-- | Post an article. Note the type signature tells you:
--   - It reads Config
--   - It modifies NewsState  
--   - It can fail with NewsError
--   - It performs IO
-- All in the types. No surprises. No segfaults.
postArticle :: MonadNews m => Article -> m ArticleId
postArticle art = do
    maxSize <- asks configMaxArticleSize
    let actualSize = length (show art)  -- Simplified
    when (actualSize > maxSize) $
        throwError $ ArticleTooLarge actualSize maxSize
    modify $ \s -> s { stateArticles = Map.insert (articleId art) art (stateArticles s) }
    pure (articleId art)

-- | Retrieve an article. Failure is explicit in the types.
getArticle :: MonadNews m => ArticleId -> m Article
getArticle aid = do
    arts <- gets stateArticles
    case Map.lookup aid arts of
        Nothing -> throwError $ ArticleNotFound aid
        Just a  -> pure a

-- | List all groups. Pure query, no mutation.
listGroups :: MonadNews m => m [Group]
listGroups = Map.elems <$> gets stateGroups

-- | Check access level. Explicit, typed, no magic numbers.
checkAccess :: MonadNews m => GroupName -> m AccessLevel
checkAccess _ = pure ReadWrite  -- TODO: Implement properly
