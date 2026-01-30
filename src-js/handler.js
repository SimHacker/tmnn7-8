/**
 * TMNN Serverless - Cloud-Native Usenet
 * 
 * "Move fast and post things" - WebScaleChad
 */

const { DynamoDBClient } = require('@aws-sdk/client-dynamodb');
const { DynamoDBDocumentClient, PutCommand, GetCommand, QueryCommand } = require('@aws-sdk/lib-dynamodb');
const { v4: uuidv4 } = require('uuid');
const _ = require('lodash');

// Initialize clients (cold start optimization coming in v2)
const client = new DynamoDBClient({});
const docClient = DynamoDBDocumentClient.from(client);

const ARTICLES_TABLE = process.env.ARTICLES_TABLE || 'tmnn-articles';
const GROUPS_TABLE = process.env.GROUPS_TABLE || 'tmnn-groups';

/**
 * POST /articles
 * 
 * Create a new article. Like the old inews, but RESTful.
 * And serverless. And webscale.
 */
module.exports.postArticle = async (event) => {
  try {
    const body = JSON.parse(event.body);
    
    // Validation (joi schema coming in v2, this is MVP)
    if (!body.subject || !body.from || !body.body) {
      return {
        statusCode: 400,
        body: JSON.stringify({ error: 'Missing required fields', fields: ['subject', 'from', 'body'] }),
      };
    }

    const article = {
      id: uuidv4(),  // Better than whatever timestamp nonsense the original used
      subject: body.subject,
      from: body.from,
      body: body.body,
      groups: body.groups || ['misc.test'],
      createdAt: new Date().toISOString(),
      // Note: No buffer overflow possible here. JSON handles strings properly.
    };

    await docClient.send(new PutCommand({
      TableName: ARTICLES_TABLE,
      Item: article,
    }));

    return {
      statusCode: 201,
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(article),
    };
  } catch (error) {
    console.error('Error posting article:', error);
    // Note: We return proper error responses, not exit(1)
    return {
      statusCode: 500,
      body: JSON.stringify({ error: 'Internal server error', message: error.message }),
    };
  }
};

/**
 * GET /articles/{id}
 * 
 * Retrieve an article by ID. RESTful. Stateless. Scalable.
 */
module.exports.getArticle = async (event) => {
  try {
    const { id } = event.pathParameters;

    const result = await docClient.send(new GetCommand({
      TableName: ARTICLES_TABLE,
      Key: { id },
    }));

    if (!result.Item) {
      return {
        statusCode: 404,
        body: JSON.stringify({ error: 'Article not found' }),
      };
    }

    return {
      statusCode: 200,
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(result.Item),
    };
  } catch (error) {
    console.error('Error getting article:', error);
    return {
      statusCode: 500,
      body: JSON.stringify({ error: 'Internal server error' }),
    };
  }
};

/**
 * GET /groups
 * 
 * List all newsgroups. With pagination! (TODO: implement pagination)
 */
module.exports.listGroups = async (event) => {
  // TODO: Actually implement this
  // For now, return hardcoded groups because we're moving fast
  return {
    statusCode: 200,
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({
      groups: [
        { name: 'comp.lang.javascript', description: 'The only language that matters' },
        { name: 'comp.cloud.aws', description: 'Serverless everything' },
        { name: 'alt.webscale', description: 'When you need to scale' },
        { name: 'misc.startups', description: 'Move fast and break things' },
      ],
      // Note: pagination coming in v2, we're in MVP mode
      _meta: { total: 4, hasMore: false }
    }),
  };
};

/**
 * Access control? What's that?
 * 
 * In the original code, there was a file called fascist.c that
 * controlled who could post. We're not doing that. This is the
 * cloud. Everyone can post. That's the point.
 * 
 * (Rate limiting coming in v2)
 */
