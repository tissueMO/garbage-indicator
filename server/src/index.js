const { promises: fs } = require('fs');
const dayjs = require('dayjs');
const express = require('express');
const cors = require('cors');
const bodyParser = require('body-parser');
const { Dayjs } = require('dayjs');

const app = express();
app.use(bodyParser.urlencoded({ extended: true }));
app.use(bodyParser.json());
app.use(cors());

dayjs.extend(require('dayjs/plugin/timezone'));
dayjs.extend(require('dayjs/plugin/utc'));
dayjs.tz.setDefault('Asia/Tokyo');

const PORT = 8080;
const MASTER_ORIGIN_PATH = 'master.origin.json';
const MASTER_PATH = '/app/data/master.json';
const MAX_DAYS_COUNT = 99;

/**
 * マスターデータを取得します。
 * @returns {Promise<Object>}
 */
const fetchMaster = async () => {
  try {
    return JSON.parse(await fs.readFile(MASTER_PATH, 'utf-8'));
  } catch (e) {
    return JSON.parse(await fs.readFile(MASTER_ORIGIN_PATH, 'utf-8'));
  }
};

/**
 * 次の収集日を取得します。
 * @param {Object} rule 収集ルール
 * @param {number[]} rule.nthWeek 第何週か
 * @param {number[]} rule.dayOfWeek 何曜日か
 * @returns {Dayjs?} 次の収集日
 */
const getNextDay = (rule) => {
  const now = dayjs().tz();

  for (let i = 0; i < MAX_DAYS_COUNT; i++) {
    const next = now.add(i, 'd');

    if (
      rule.dayOfWeek.includes(next.day()) &&
      rule.nthWeek.includes(Math.floor((next.date() + 6) / 7))
    ) {
      return next;
    }
  }

  return null;
};

app
  /**
   * マスターデータを返します。
   */
  .get('/master.json', async (_, res) => {
    return res.json(await fetchMaster());
  })

  /**
   * マスターデータを更新します。
   */
  .put('/master.json', async (req, res) => {
    const json = req.body;
    const data = JSON.stringify(json, null, 2);

    try {
      await fs.writeFile(MASTER_PATH, data, { encoding: 'utf-8' });
      res.sendStatus(200);
    } catch (e) {
      console.error(`ERROR: マスターデータの書き込みに失敗しました。data=${data}, e=${e}`);
      console.log(e.stack);
      res.sendStatus(500);
    }
  })

  /**
   * 指定したごみ種別の収集日までの残り日数を返します。
   */
  .get('/check/:type', async (req, res) => {
    const master = await fetchMaster();
    const type = req.params.type;

    if (!Object.keys(master).includes(type)) {
      res.sendStatus(400);
      return;
    }

    // 次の収集日を算出
    const now = dayjs().tz();
    const rule = master[type];
    const next = getNextDay(rule) ?? now.add(MAX_DAYS_COUNT, 'd');
    const deltaDays = next.diff(now, 'day');
    res
      .set('Content-Type', 'text/plain')
      .status(200)
      .send(`${deltaDays}`);
  })
;


// サーバー起動
app.listen(PORT);
console.log(`Server Started on port ${PORT}`);
